


# Sample scripts

```javascript

/*

job:
	location: 'job'|'local'|'remote'
	args: string[] - Array of command line arguments. First argument is a command name.
  command: string | null - Destination tool
  execOnJob(args?: string[]) - Execute command on this job process
  execOnLocal(options: ExecOptions) - Execute JS script in Node.js on local controller
  execOnRemote(options: ExecOptions) - Execute JS script in Node.js on remote server
*/

////////////////// clang.js - executed in QuickJS on this job process

function clang() {
  if (job.args.find('-c') < 0) return job.execLocal();
  if (job.getPrefferLocal()) return job.execLocal();
  let lastArg = null;
  let output = null;
  for (let arg of job.args) {
    if (lastArg === '-o') {
      output = arg;
    }
    lastArg = arg;
  }
  if (output === null) return job.execLocal();
  let ppArgs = job.args.map(arg => arg === '-c' ? '-E' : arg);
  ppArgs[0] = job.command;
  job.execLocal(ppArgs);
  let res = job.execRemote({
  	remote: 'clang-remote.js/clangRemote',
    input: new job.File(output, true, false),
    output: new job.File(output, false, true),
  });
}

job.register(clang, {
  name: 'clang',
  before: 'gcc',
  commands: {
  	'clang': 'clang',
  	'clang++': 'clang++'
  }
});


////////////////// clang-remote.js

function clangRemote(input) {

}

```
