
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawn } = require('child_process');


function strarr(strings, ...values) {
    let raw = strings.raw;
    let str = raw[0];
    for (let i = 1; i < raw.length; i++) {
        let val = values[i - 1];
        if (typeof (val) == 'object' && val instanceof Array) {
            val = val
                .map((x) => {
                    x = JSON.stringify(x).substr(1);
                    return x.substr(0, x.length - 1);
                })
                .join('\n');
            val = `\n${val}\n`;
        } else if (typeof (val) == 'undefined') {
            val = '';
        }
        str += val;
        str += raw[i];
    }
    let arr = [];
    for (let line of str.split(/[\s\r]*\n[\s\r]*/)) {
        if (line == '') continue;
        arr.push(JSON.parse(`"${line}"`));
    }
    return arr;
}

function parseArgs(switches) {
    for (let i = 2; i < process.argv.length; i++) {
        let arg = process.argv[i];
        let ret;
        if (arg.startsWith('--')) {
            ret = switches[arg.substr(2)](i, arg, process.argv[i + 1]);
        } else {
            ret = switches['_'](i, arg, process.argv[i + 1]);
        }
        if (typeof (ret) != 'undefined') {
            i += ret;
        }
    }
}

function mkdirWithParents(dirs) {
    let full = '';
    for (dir of dirs.split(/[\\\/]+/g)) {
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

class Compiler {

    constructor() {

        this.doClean = false;
        this.doBuild = true;
        this.maxProcesses = os.cpus().length;
        this.variant = '';
        this.inputDir = '.';
        this.objectDir = 'build/@';
        this.outputDir = 'build/@';
        this.target = 'a';
        this.options = {};
        this.src = [];
        this.objFiles = [];
        this.fatal = false;
        this.processCount = 0;
        this.processCountWait = new WaitCondition();

        this.parseArgs();
    }

    parseArgs() {
        let sw = {
            clean: (i, arg, nextArg) => {
                this.doClean = true;
                this.doBuild = false;
            },
            rebuild: (i, arg, nextArg) => {
                this.doClean = true;
                this.doBuild = true;
            },
            jobs: (i, arg, nextArg) => {
                this.maxProcesses = parseInt(nextArg);
                return 1;
            },
            input: (i, arg, nextArg) => {
                this.inputDir = nextArg;
                return 1;
            },
            object: (i, arg, nextArg) => {
                this.objectDir = nextArg;
                return 1;
            },
            output: (i, arg, nextArg) => {
                this.outputDir = nextArg;
                return 1;
            },
            _: (i, arg, nextArg) => {
                let pos = arg.indexOf('=');
                if (pos > 0) {
                    this.options[arg.substr(0, pos)] = JSON.parse(arg.substr(pos + 1));
                } else {
                    this.variant = arg;
                }
            }
        };
        parseArgs(sw);
    }

    resolvePath(path) {
        return path.replace('@', this.variant);
    }

    async eachFile(callback) {
        for (let file of this.src) {
            let srcFile = this.resolvePath(path.join(this.inputDir, file));
            let objFile = this.resolvePath(path.join(this.objectDir, `${file}.o`));
            let depFile = this.resolvePath(path.join(this.objectDir, `${file}.dep`));
            mkdirWithParents(path.dirname(objFile));
            let p = callback.call(this, srcFile, objFile, depFile);
            await p;
            if (this.fatal) break;
        }
        await this.processCountWait.wait(() => (this.processCount == 0));
    }

    async singleRemove(srcFile, objFile, depFile) {
        try {
            fs.unlinkSync(objFile);
        } catch (ex) { }
        try {
            fs.unlinkSync(depFile);
        } catch (ex) { }
    }

    exec(cmd, args) {
        console.log(`${cmd} ${JSON.stringify(args)}`);
        this.processCount++;
        const proc = spawn(cmd, args);

        proc.stdout.on('data', (data) => {
            console.log(`stdout: ${data}`);//TODO: stdout
        });
        proc.stderr.on('data', (data) => {
            console.log(`stderr: ${data}`);//TODO: stderr
        });
        proc.on('close', (code) => {
            this.processCount--;
            if (code != 0) this.fatal = true;
            this.processCountWait.fire();
        });
        proc.on('error', (error) => {
            console.log(error);
            this.fatal = true;
            this.processCountWait.fire();
        });
    }

    parseDepFile(depFile) {
        try {
            let cnt = fs.readFileSync(depFile, 'UTF-8')
                .replace(/\\\r?\n/g, ' ')
                .split(/[\s\r]*\n[\s\r]*/);
            let r = [];
            for (let line of cnt) {
                let pos = line.indexOf(': ');
                if (pos < 0) continue;
                let files = line.substr(pos + 1)
                    .split(/\s+/);
                r = r.concat(files);
            }
            return r;
        } catch (ex) {
            return null;
        }
    }

    getTime(file) {
        try {
            let stat = fs.statSync(file);
            return stat.mtimeMs;
        } catch (ex) {
            return null;
        }
    }

    resolveDeps(target, sources, depFile) {
        let deps = new Set();
        if (depFile) {
            let list = this.parseDepFile(depFile);
            if (list === null) return true;
            for (let file of list) {
                if (file != '') {
                    deps.add(file);
                }
            }
        }
        if (sources) {
            for (let file of sources) {
                deps.add(file);
            }
        }
        let targetTime = this.getTime(target);
        if (targetTime === null) {
            return true;
        }
        for (let file of deps) {
            let fileTime = this.getTime(file);
            if (fileTime === null || fileTime >= targetTime) {
                return true;
            }
        }
        return false;
    }

    async singleCompile(srcFile, objFile, depFile) {

        this.objFiles.push(objFile);

        await this.processCountWait.wait(() => (this.processCount < this.maxProcesses || this.fatal));
        if (this.fatal) return;

        let update = this.resolveDeps(objFile, [srcFile], depFile);

        if (update) {
            let args = this.onCompile(srcFile, objFile, depFile);
            let cmd = args.shift();
            console.log(`Compiling: ${srcFile}`);
            this.exec(cmd, args);
        } else {
            console.log(`Up to date: ${srcFile}`);
        }
    }

    async link() {
        await this.processCountWait.wait(() => (this.processCount < this.maxProcesses || this.fatal));
        if (this.fatal) return;

        let targetFile = this.resolvePath(path.join(this.outputDir, this.target));

        let update = this.resolveDeps(targetFile, this.objFiles);

        if (update) {
            let args = this.onLink(this.objFiles, targetFile);
            let cmd = args.shift();
            console.log(`Linking: ${targetFile}`);
            this.exec(cmd, args);
        } else {
            console.log(`Up to date: ${targetFile}`);
        }
    }

    async compile() {
        if (this.doClean) {
            await this.eachFile(this.singleRemove);
        }
        if (this.doBuild) {
            await this.eachFile(this.singleCompile);
            await this.link();
        }
    }
};

exports.strarr = strarr;
exports.Compiler = Compiler;
