# Excalidraw Prompts

Export the generated diagrams to these files:

- `images/image1.png`
- `images/image2.png`
- `images/image3.png`

## image1.png

Create a clean technical diagram for a project called "splinter", a fully external JVM analysis toolkit.

Show these blocks left to right:

1. "splinter (external process)"
2. "windows process / memory reader"
3. "target javaw.exe"
4. "jvm.dll"
5. "HotSpot metadata"

Inside "HotSpot metadata", show smaller nodes:

- VMStructs
- VMTypes
- ConstantPool
- Klass
- InstanceKlass
- Method
- ConstMethod

Add arrows:

- splinter -> windows process / memory reader : attach/read
- windows process / memory reader -> target javaw.exe : open process
- target javaw.exe -> jvm.dll : module loaded
- jvm.dll -> VMStructs : exported metadata
- splinter -> ConstantPool / Klass / Method / ConstMethod : decode live runtime structures

Style:

- minimal black and white
- developer documentation style
- rectangular boxes
- clear directional arrows
- title: "splinter high-level architecture"

## image2.png

Create a vertical pipeline diagram titled "current splinter runtime pipeline".

Top to bottom steps:

1. discover javaw.exe
2. open target process
3. find jvm.dll
4. resolve VMStruct exports
5. parse VMStructs / VMTypes / constants
6. walk ClassLoaderDataGraph
7. inspect Klass / InstanceKlass
8. decode ConstantPool
9. inspect Method / ConstMethod
10. read and print bytecode

Show arrows between each step.
On the right side, add outputs:

- loaded classes
- fields
- methods
- constant pool entries
- bytecode dump

Style:

- clean engineering flowchart
- monochrome
- compact enough for a README

## image3.png

Create a technical diagram titled "HotSpot rewritten bytecode resolution in splinter".

Show these nodes:

- "ConstMethod bytecode stream"
- "rewritten HotSpot opcodes"
- "ConstantPoolCache"
- "resolved field/method/indy entries"
- "ConstantPool"
- "symbolic output"

Under "rewritten HotSpot opcodes", include examples:

- fast_iaccess_0
- fast_igetfield
- fast_aldc
- invokehandle
- invokedynamic

Arrows:

- ConstMethod bytecode stream -> rewritten HotSpot opcodes
- rewritten HotSpot opcodes -> ConstantPoolCache : runtime index lookup
- ConstantPoolCache -> resolved field/method/indy entries
- resolved field/method/indy entries -> ConstantPool : original cp index
- ConstantPool -> symbolic output : class / method / field names

Add a small note:

"runtime bytecodes are mapped back to symbolic references"

Style:

- white background
- black connectors
- precise documentation diagram
- readable in a GitHub README
