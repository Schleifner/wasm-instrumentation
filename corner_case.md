## code map issue between source ts and wasm binary

### @inline function case
`a.ts` -> inline foo():bool  
`b.ts` -> bar()  
```typescript
// source code of bar()
bar(){
    let i:i32=0;
    if(foo()){
        i++;
    } else {
        i--;
    }
}
```

branch info shoule be: `[[1, 2], [1.3]]`  
debug info: `[1:["@a.ts:1:1"], 2:["@b.ts:4:10"], 3:["@b.ts:6:10"]]`  
<b>Q</b>: if we exclude the `a.ts` file from the report, how could we handle basic block 1?  

<b>S</b>: bar() is in b.ts, we do the exclude and include based on the function name, function name contains the file information, like `~lib/rt/itcms/Object#set:color`.  

we will instrument the full `bar()` function, and double filter the run trace info in the front end.  

