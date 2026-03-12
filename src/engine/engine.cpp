#include "engine.h"

#include "hotspot/classLoaderData.h"

namespace splinter::engine {
  bool engine::initialize() {
    lastError_.clear();

    if (!process_.attachToJavaw()) {
      lastError_ = process_.lastError();
      return false;
    }

    if (!vm_.refresh(process_)) {
      lastError_ = vm_.lastError();
      return false;
    }

    return true;
  }

  const std::string &engine::lastError() const noexcept {
    return lastError_;
  }

  const memory::remoteProcess &engine::process() const noexcept {
    return process_;
  }

  const hotspot::vmStructs &engine::vm() const noexcept {
    return vm_;
  }

  memory::processMemory engine::memory() const noexcept {
    return memory::processMemory(process_);
  }

  const analyzer::classAnalyzer &engine::classes() const noexcept {
    return classAnalyzer_;
  }

  const analyzer::methodAnalyzer &engine::methods() const noexcept {
    return methodAnalyzer_;
  }

  const analyzer::fieldAnalyzer &engine::fields() const noexcept {
    return fieldAnalyzer_;
  }

  const analyzer::bytecodeAnalyzer &engine::bytecode() const noexcept {
    return bytecodeAnalyzer_;
  }

  hotspot::symbolTable engine::symbols() const noexcept {
    return hotspot::symbolTable(memory(), vm_);
  }

  std::vector<std::uint64_t> engine::loadedKlassAddresses(std::size_t limit) const {
    const auto processMemory = memory();
    return hotspot::classLoaderDataView::enumerate(processMemory, vm_, 0, limit);
  }
}
