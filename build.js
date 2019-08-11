const { strarr, Compiler } = require('./build_utils.js');

let c = new Compiler();

switch (c.variant) {
    case 'cli':
        c.target = process.platform == 'win32' ? 'mjsc.exe' : 'mjsc';
        c.src = strarr`${c.src}
            cli.c`;
        break;
    case 'web':
        c.target = 'mjscompile';
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

if (0) {

    //---------------------------------

    let cSrc = []; // C sources
    let flags = []; // flags for both linker and compiler
    let compileFlags = ['-I.', '-Imjs/src', '-Ifrozen']; // compiler flags
    let linkFlags = []; // linker flags
    let defines = ['CS_PLATFORM=0', 'MJS_EXPOSE_PRIVATE', 'CS_ENABLE_STDIO=0', 'JSON_MINIMAL']; // defines

    let withParser;

    let variant = 'cli'; // web, cli, lib

    cSrc = strarr`
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
    mjs/src/ffi/ffi.c
`;

    defines = defines.concat([(c) => c.file == 'mjs/src/mjs_primitive.c' ? 'SOMEDEF=123' : null]);

    cc = (c) => c.variant == 'web' ? 'emcc' :
        c.variant == 'lib' ? 'arm-none-eabi-gcc' :
            c.variant == 'cli' ? 'gcc' :
                errorValue();

    if (withParser) {
        cSrc = strarr`
        ${cSrc}
        mjs/src/mjs_parser.c
        mjs/src/mjs_tok.c
    `;
        defines.concat(['WITH_PARSER']);
    }

    let cSrcArray = [];
    let cObjArray = [];

    function mkdirWithParents(dirs) {
        let full = '';
        for (dir of dirs.split('/')) {
            full += dir;
            if (!fs.existsSync(full)) {
                fs.mkdirSync(full);
            }
            full += '/';
        }
    }

    class WaitCondition {

        constructor() {
            this.resolves = [];
        }

        async wait(cond) {
            if (cond()) return;
            return new Promise((resolve) => {
                this.resolves.push([cond, resolve]);
            });
        }

        fire() {
            let i = this.resolves.length;
            while (i--) {
                if (this.resolves[i][0]()) {
                    let r = this.resolves[i][1];
                    this.resolves.splice(i, 1);
                    r();
                }
            }
        }
    }

    let fatal = false;
    let compilationCount = 0;
    let compilationCountWait = new WaitCondition();
    let maxCount = 4;

    let start = Date.now();

    function ms() {
        return (Date.now() - start);
    }

    async function compile(srcFile, objFile, depFile) {
        await compilationCountWait.wait(() => (compilationCount < maxCount || fatal));
        if (fatal) return;

        if (fs.existsSync(objFile) && fs.existsSync(depFile)) {
            let genTime = Math.min(getTime(objFile), getTime(depFile));
            let rebuild = getTime(srcFile) >= genTime;
            if (!rebuild) {
                files = parseDepFile(depFile);
                for (let file in files) {
                    if (getTime(file) >= genTime) {
                        rebuild = true;
                        break;
                    }
                }
            }
            if (!rebuild) {
                console.log(`${ms()} Skipping ${srcFile}`);
                return;
            }
        }

        compilationCount++;
        console.log(`${ms()} Compiling ${srcFile}...`);


        const args = ['-c'].concat(flags, compileFlags, defines.map((x) => `-D${x}`), ['-MMD', '-MF', depFile, srcFile, '-o', objFile]);
        const proc = spawn('gcc', args);
        console.log(`gcc ${JSON.stringify(args)}`);
        proc.stdout.on('data', (data) => {
            console.log(`stdout: ${data}`);
        });
        proc.stderr.on('data', (data) => {
            console.log(`stderr: ${data}`);
        });
        proc.on('close', (code) => {
            compilationCount--;
            if (code != 0) fatal = true;
            compilationCountWait.fire();
        });
        proc.on('error', (error) => {
            console.log(`ERROR: ${error}`);
            fatal = true;
            compilationCountWait.fire();
        });

        /*return new Promise((resolve, reject) => {
    
            const ls = spawn('ls', ['-lh', '/usr']);
            ls.stdout.on('data', (data) => {
                console.log(`stdout: ${data}`);
            });
            ls.stderr.on('data', (data) => {
                console.log(`stderr: ${data}`);
            });
            ls.on('close', (code) => {
                console.log(`child process exited with code ${code}`);
            });
            ls.on('error', (error) => {
                reject(error);
            });
        });*/
    }

    async function compileAll() {
        for (let srcFile of cSrc.split(/[\s\r]*\n[\s\r]*/)) {
            if (srcFile == '') continue;
            let objFile = `build/${variant}/${srcFile}.o`;
            let depFile = `build/${variant}/${srcFile}.dep`;
            cSrcArray.push(srcFile);
            cObjArray.push(objFile);
            mkdirWithParents(path.dirname(objFile));
            await compile(srcFile, objFile, depFile);
            if (fatal) break;
        }
        await compilationCountWait.wait(() => (compilationCount == 0));
        console.log(`done ${ms()}`);
    }

    async function main() {
        await compileAll();
    }

    main();


    //console.log(JSON.stringify(cSrcArray, null, 4));
    //console.log(JSON.stringify(cObjArray, null, 4));

}
