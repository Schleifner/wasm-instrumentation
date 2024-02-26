Wasm-instrumentation
====================

`Wasm-instrumentation` is a WebAssembly instrument tool using `binaryen` to implement function mock and coverage collection.

Install wasm-instrumentation
----------------------------

```bash
npm i -D wasm-instrumentation
```

How to use
----------

```js

import initInstrumenter from "wasm-instrumentation";

export async function instrument() {
  const instrumenter = await initInstrumenter();
  const source = instrumenter.allocateUTF8("./build.wasm");
  const output = instrumenter.allocateUTF8("./build.instrumented.wasm");
  const report = instrumenter.allocateUTF8("covInstrument/traceExpression");
  const sourceMap = instrumenter.allocateUTF8("./build.wasm.map");
  const debugInfo = instrumenter.allocateUTF8("./build.debugInfo.json");
  const expectInfo = instrumenter.allocateUTF8("./build.expectInfo.json");

  instrumenter._cdc_instrument(source, output, report, sourceMap, expectInfo, debugInfo, null, null, true);
}
```

API
---

```js
_cdc_instrument(sourceWasm, outputWasm, reportFunctionName, sourceMap, expectInfo, debugInfo, include, exclude, skipLib)
```
- sourceWasm: source wasm file path
- outputWasm: output wasm file path after instrumentation
- reportFunctionName: instrument function, default: "covInstrument/traceExpression"
- sourceMap: source map file of source wasm
- expectInfo: expectInfo file path after instrumentation
- debugInfo: debugInfo file path after instrumentation
- includes: An array of Regex indicate which function shoule be included. eg. ["assembly/.*","src/.*"]
- excludes: An array of Regex indicate which function shoule be excluded. eg. ["test/.*"]
- skipLib: if skip the Assemblyscript lib functions
