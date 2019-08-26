const { strarr, Compiler } = require('./build_utils.js');

let c = new Compiler();

switch (c.variant) {
    case 'cli':
        c.target = process.platform == 'win32' ? 'mjsc.exe' : 'mjsc';
        c.src = strarr`${c.src}
            cli.c`;
        c.defines = strarr`${c.defines}
            WITH_PARSER`;
        break;
    case 'web':
        c.target = 'mjscompile';
        c.defines = strarr`${c.defines}
            WITH_PARSER`;
        break;
    case 'lib':
        c.target = 'mjs.a';
        break;
}

c.outputDir = 'build';

c.src = strarr`${c.src}
    common/cs_varint.c
    common/mbuf.c
    common/mg_str.c
    common/str_util.c
    frozen/frozen.c
    mjs/src/mjs_array.c
    mjs/src/mjs_bcode.c
    mjs/src/mjs_builtin.c
    mjs/src/mjs_conversion.c
    mjs/src/mjs_core.c
    mjs/src/mjs_exec.c
    mjs/src/mjs_ffi.c
    mjs/src/mjs_gc.c
    mjs/src/mjs_object.c
    mjs/src/mjs_primitive.c
    mjs/src/mjs_string.c
    mjs/src/mjs_util.c
    mjs/src/ffi/ffi.c`;

if (c.variant != 'lib') {
    c.src = strarr`${c.src}
        mjs/src/mjs_parser.c
        mjs/src/mjs_tok.c`;
}

c.defines = strarr`${c.defines}
    CS_PLATFORM=0
    MJS_EXPOSE_PRIVATE
    CS_ENABLE_STDIO=0
    JSON_MINIMAL`;

c.includes = strarr`${c.includes}
    .
    mjs/src
    frozen`;

c.flags = strarr`${c.flags}
    -g`;

c.compileFlags = strarr`${c.compileFlags}`;

c.linkFlags = strarr`${c.linkFlags}`;

if (c.options.debug) {
    c.flags = strarr`${c.flags}
        -O0`;
    c.defines = strarr`${c.defines}
        DEBUG`;
} else {
    c.flags = strarr`${c.flags}
        -Os`;
    c.defines = strarr`${c.defines}
        NDEBUG`;
}

c.onCompile = function (srcFile, objFile, depFile) {
    let compiler;
    switch (c.variant) {
        case 'cli': compiler = 'gcc'; break;
        case 'lib': compiler = 'arm-none-eabi-gcc'; break;
        case 'web': compiler = 'emcc'; break;
        default: throw Error('Invalid variant');
    }
    return [compiler, '-c']
        .concat(c.flags)
        .concat(c.compileFlags)
        .concat(c.defines.map((x) => `-D${x}`))
        .concat(c.includes.map((x) => `-I${x}`))
        .concat(['-MMD', '-MF', depFile])
        .concat([srcFile, '-o', objFile]);
}

c.onLink = function (objFiles, targetFile) {
    let compiler;
    switch (c.variant) {
        case 'cli': compiler = 'gcc'; break;
        case 'lib': compiler = 'arm-none-eabi-gcc'; break;
        case 'web': compiler = 'emcc'; break;
        default: throw Error('Invalid variant');
    }
    return [compiler]
        .concat(c.flags)
        .concat(c.linkFlags)
        .concat(objFiles)
        .concat(['-o', targetFile]);
}

c.compile();
