#include "engine/engine.h"

#include "engine/hotspot/constantPool.h"
#include "engine/hotspot/instanceKlass.h"

#include <charconv>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
    namespace splinterEngine = splinter::engine;

    struct options {
        std::uint32_t pid = 0;
        std::vector<std::wstring> processNames;
        bool listProcesses = false;
        bool info = false;
        bool listClasses = false;
        bool constants = false;
        bool disassemble = false;
        std::string search;
        std::string className;
        std::string methodName;
        std::string descriptor;
        std::size_t limit = 50;
        bool help = false;
    };

    void printUsage() {
        std::cout <<
                "splinter, a jvm runtime analysis toolkit\n"
                "\n"
                "usage: splinter [options]\n"
                "\n"
                "target selection:\n"
                "  --pid <id>            attach to a specific process id\n"
                "  --process <name>      image name to look for, repeatable\n"
                "                        (defaults to javaw.exe then java.exe)\n"
                "  --list-processes      show every java process and whether it can be read\n"
                "\n"
                "queries:\n"
                "  --info                target, vmstruct and index summary (default)\n"
                "  --list-classes        list loaded classes\n"
                "  --search <text>       list loaded classes whose name contains text\n"
                "  --class <name>        inspect one class, use the internal name\n"
                "                        (java/lang/String)\n"
                "  --method <name>       with --class, inspect matching methods\n"
                "  --descriptor <desc>   narrow --method to one descriptor\n"
                "  --constants           dump the selected class constant pool\n"
                "  --disasm              disassemble the selected methods\n"
                "  --limit <n>           cap list output, 0 for no cap (default 50)\n"
                "  --help                show this text\n";
    }

    [[nodiscard]] bool parseSize(std::string_view text, std::size_t &out) {
        const auto result = std::from_chars(text.data(), text.data() + text.size(), out);
        return result.ec == std::errc() && result.ptr == text.data() + text.size();
    }

    [[nodiscard]] bool parseArguments(int argc, char **argv, options &parsed, std::string &error) {
        const auto valueFor = [&](int &index, std::string_view flag, std::string &out) {
            if (index + 1 >= argc) {
                error = std::format("{} needs a value", flag);
                return false;
            }
            out = argv[++index];
            return true;
        };

        for (int index = 1; index < argc; ++index) {
            const std::string_view argument = argv[index];
            std::string value;

            if (argument == "--help" || argument == "-h") {
                parsed.help = true;
            } else if (argument == "--pid") {
                if (!valueFor(index, argument, value)) {
                    return false;
                }
                std::size_t pid = 0;
                if (!parseSize(value, pid) || pid == 0) {
                    error = std::format("{} is not a valid process id", value);
                    return false;
                }
                parsed.pid = static_cast<std::uint32_t>(pid);
            } else if (argument == "--process") {
                if (!valueFor(index, argument, value)) {
                    return false;
                }
                parsed.processNames.push_back(splinterEngine::memory::widen(value));
            } else if (argument == "--list-processes") {
                parsed.listProcesses = true;
            } else if (argument == "--info") {
                parsed.info = true;
            } else if (argument == "--list-classes") {
                parsed.listClasses = true;
            } else if (argument == "--search") {
                if (!valueFor(index, argument, parsed.search)) {
                    return false;
                }
            } else if (argument == "--class") {
                if (!valueFor(index, argument, parsed.className)) {
                    return false;
                }
            } else if (argument == "--method") {
                if (!valueFor(index, argument, parsed.methodName)) {
                    return false;
                }
            } else if (argument == "--descriptor") {
                if (!valueFor(index, argument, parsed.descriptor)) {
                    return false;
                }
            } else if (argument == "--constants") {
                parsed.constants = true;
            } else if (argument == "--disasm") {
                parsed.disassemble = true;
            } else if (argument == "--limit") {
                if (!valueFor(index, argument, value) || !parseSize(value, parsed.limit)) {
                    error = error.empty() ? std::format("{} is not a valid limit", value) : error;
                    return false;
                }
            } else {
                error = std::format("unknown argument {}", argument);
                return false;
            }
        }

        return true;
    }

    int listProcesses() {
        const auto candidates = splinterEngine::memory::enumerateJavaProcesses();
        if (candidates.empty()) {
            std::cout << "no java processes found\n";
            return 1;
        }

        for (const auto &candidate: candidates) {
            std::cout << std::format("pid={} {} readable={} jvm={}\n",
                                     candidate.pid,
                                     splinterEngine::memory::narrow(candidate.name),
                                     candidate.readable ? "yes" : "no",
                                     candidate.hasJvm ? "yes" : "no");
        }
        return 0;
    }

    void printInfo(const splinterEngine::engine &engine) {
        std::cout << engine.process().describeTarget() << '\n'
                << std::format("vmStructs fields={} types={} intConstants={} longConstants={}\n",
                               engine.vm().fields().size(),
                               engine.vm().types().entries().size(),
                               engine.vm().constants().intEntries().size(),
                               engine.vm().constants().longEntries().size());

        const auto &diagnostics = engine.diagnostics();
        std::cout << std::format("index klasses={} classes={} methods={} fields={} "
                                 "skippedClasses={} skippedMembers={}\n",
                                 diagnostics.klassesSeen,
                                 diagnostics.classes,
                                 diagnostics.methods,
                                 diagnostics.fields,
                                 diagnostics.klassesSkipped,
                                 diagnostics.membersSkipped);
        if (!diagnostics.firstSkipReason.empty()) {
            std::cout << "first skip: " << diagnostics.firstSkipReason << '\n';
        }
    }

    void printClassList(const std::vector<splinterEngine::classInfo> &classes, std::size_t total) {
        for (const auto &entry: classes) {
            std::cout << std::format("0x{:X} {} {} methods={}\n",
                                     entry.address,
                                     entry.isInstanceKlass() ? "instance" : "other   ",
                                     entry.name,
                                     entry.methodCount);
        }

        if (total > classes.size()) {
            std::cout << std::format("... {} more, raise --limit to see them\n", total - classes.size());
        }
    }

    void printMethodDetail(const splinterEngine::engine &engine, const splinterEngine::methodInfo &method) {
        const auto details = engine.describeMethod(method);
        if (!details) {
            return;
        }

        std::cout << std::format("    code={} maxStack={} maxLocals={} lines={} locals={} handlers={} "
                                 "checked={} params={}\n",
                                 details->codeSize.value_or(0),
                                 details->maxStack.value_or(0),
                                 details->maxLocals.value_or(0),
                                 details->lineNumbers.size(),
                                 details->localVariables.size(),
                                 details->exceptionHandlers.size(),
                                 details->checkedExceptions.size(),
                                 details->parameters.size());

        if (!details->genericSignature.empty()) {
            std::cout << "    generic " << details->genericSignature << '\n';
        }

        for (const auto &handler: details->exceptionHandlers) {
            std::cout << std::format("    handler [{},{}) -> {} catch {}\n",
                                     handler.startPc, handler.endPc, handler.handlerPc,
                                     handler.catchType.empty() ? "any" : handler.catchType);
        }
    }

    void printDisassembly(const splinterEngine::engine &engine, const splinterEngine::methodInfo &method) {
        const auto disassembled = engine.disassemble(method);
        if (!disassembled) {
            std::cout << "    no bytecode\n";
            return;
        }

        std::cout << std::format("    {} instructions, operands read as {}\n",
                                 disassembled->instructions.size(),
                                 disassembled->rewritten ? "rewritten" : "classfile");
        for (const auto &instruction: disassembled->instructions) {
            std::cout << std::format("    {:>5}: {}", instruction.offset, instruction.mnemonic);
            if (!instruction.operandText.empty()) {
                std::cout << ' ' << instruction.operandText;
            }
            std::cout << '\n';
        }
    }

    void printMethods(const splinterEngine::engine &engine, const options &parsed) {
        auto methods = parsed.methodName.empty()
                           ? engine.methodsForClass(parsed.className)
                           : engine.findMethods(parsed.className, parsed.methodName);

        if (!parsed.descriptor.empty()) {
            std::erase_if(methods, [&](const auto &method) { return method.descriptor != parsed.descriptor; });
        }

        std::cout << std::format("methods: {}\n", methods.size());
        std::size_t shown = 0;
        for (const auto &method: methods) {
            if (parsed.limit != 0 && shown++ >= parsed.limit) {
                std::cout << std::format("... {} more, raise --limit to see them\n", methods.size() - parsed.limit);
                break;
            }

            const splinter::engine::classfile::accessFlags flags(method.accessFlags.value_or(0));
            const auto modifiers = flags.describe();
            std::cout << std::format("  0x{:X} {}{}{} {}\n",
                                     method.address,
                                     modifiers,
                                     modifiers.empty() ? "" : " ",
                                     method.name,
                                     method.displaySignature);

            if (!parsed.methodName.empty()) {
                printMethodDetail(engine, method);
            }
            if (parsed.disassemble) {
                printDisassembly(engine, method);
            }
        }
    }

    void printFields(const splinterEngine::engine &engine, const options &parsed) {
        const auto fields = engine.fieldsForClass(parsed.className);
        std::cout << std::format("fields: {}\n", fields.size());

        std::size_t shown = 0;
        for (const auto &field: fields) {
            if (parsed.limit != 0 && shown++ >= parsed.limit) {
                std::cout << std::format("... {} more, raise --limit to see them\n", fields.size() - parsed.limit);
                break;
            }

            const auto modifiers = field.accessFlags.describe();
            std::cout << std::format("  {}{}{} {} offset={}",
                                     modifiers,
                                     modifiers.empty() ? "" : " ",
                                     field.displayType,
                                     field.name,
                                     field.offset);
            if (field.flags.isInjected()) {
                std::cout << " injected";
            }
            if (field.flags.isStable()) {
                std::cout << " stable";
            }
            if (field.flags.isContended()) {
                std::cout << std::format(" contended({})", field.contentionGroup);
            }
            if (!field.genericSignature.empty()) {
                std::cout << " generic=" << field.genericSignature;
            }
            std::cout << '\n';
        }
    }

    void printConstantPool(const splinterEngine::engine &engine, const splinterEngine::classInfo &klass,
                           std::size_t limit) {
        const auto processMemory = engine.memory();
        const auto symbols = engine.symbols();
        const splinter::engine::hotspot::instanceKlassView view(processMemory, engine.vm(), klass.address);

        const auto constantPoolAddress = view.constantsAddress();
        if (!constantPoolAddress || *constantPoolAddress == 0) {
            std::cout << "no constant pool\n";
            return;
        }

        const splinter::engine::hotspot::constantPoolView constantPool(processMemory, engine.vm(),
                                                                       *constantPoolAddress);
        const auto entries = constantPool.decodeAll(symbols, limit);
        std::cout << std::format("constant pool: {} decoded of {}\n",
                                 entries.size(), constantPool.length().value_or(0));
        for (const auto &entry: entries) {
            std::cout << std::format("  #{} tag={} {}\n",
                                     entry.index, static_cast<unsigned>(entry.tag), entry.summary);
        }
    }

    int printClass(const splinterEngine::engine &engine, const options &parsed) {
        const auto klass = engine.findClass(parsed.className);
        if (!klass) {
            std::cout << std::format("class {} is not loaded, try --search {}\n",
                                     parsed.className, parsed.className);
            return 1;
        }

        std::cout << std::format("class {}\n  address 0x{:X}\n  kind    {}\n",
                                 klass->name,
                                 klass->address,
                                 klass->isInstanceKlass() ? "instance" : "non instance");
        if (klass->layoutHelper) {
            std::cout << std::format("  layout  {}\n", *klass->layoutHelper);
        }
        if (klass->javaFieldCount) {
            std::cout << std::format("  fields  java={} total={}\n",
                                     *klass->javaFieldCount, klass->totalFieldCount.value_or(0));
        }

        if (parsed.constants) {
            printConstantPool(engine, *klass, parsed.limit);
            return 0;
        }

        if (parsed.methodName.empty() && !parsed.disassemble) {
            printFields(engine, parsed);
        }
        printMethods(engine, parsed);
        return 0;
    }
}

int main(int argc, char **argv) {
    options parsed{};
    std::string error;
    if (!parseArguments(argc, argv, parsed, error)) {
        std::cerr << error << "\nrun splinter --help for usage\n";
        return 2;
    }

    if (parsed.help) {
        printUsage();
        return 0;
    }

    if (parsed.listProcesses) {
        return listProcesses();
    }

    splinterEngine::engine engine;
    if (!engine.initialize(splinterEngine::memory::attachOptions{parsed.pid, parsed.processNames})) {
        std::cerr << engine.lastError() << '\n'
                << "run splinter --list-processes to see what is available\n";
        return 1;
    }

    if (!parsed.className.empty()) {
        return printClass(engine, parsed);
    }

    if (!parsed.search.empty() || parsed.listClasses) {
        const auto matches = engine.searchClasses(parsed.search, parsed.limit);
        const auto total = engine.classIndex().size();
        std::cout << std::format("classes: {} loaded\n", total);
        printClassList(matches, parsed.search.empty() ? total : matches.size());
        return 0;
    }

    // default view, forces the indexes so the counts are meaningful
    static_cast<void>(engine.classIndex());
    printInfo(engine);
    if (!engine.lastError().empty()) {
        std::cerr << engine.lastError() << '\n';
        return 1;
    }
    return 0;
}
