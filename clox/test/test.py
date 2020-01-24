# Code adapted from https://github.com/munificent/craftinginterpreters/blob/master/util/test.py
# The above code is licensed under the MIT license

import re

from collections import defaultdict
from os import listdir
from os.path import abspath, basename, dirname, isdir, isfile, join, realpath, relpath, splitext
import re
from subprocess import Popen, PIPE
import sys

import term

# Runs the tests.
REPO_DIR = dirname(dirname(dirname(realpath(__file__))))

OUTPUT_EXPECT = re.compile(r'// expect: ?(.*)')
ERROR_EXPECT = re.compile(r'// (Error.*)')
ERROR_LINE_EXPECT = re.compile(r'// \[((java|c) )?line (\d+)\] (Error.*)')
RUNTIME_ERROR_EXPECT = re.compile(r'// expect runtime error: (.+)')
SYNTAX_ERROR_RE = re.compile(r'\[.*line (\d+)\] (Error.+)')
STACK_TRACE_RE = re.compile(r'\[line (\d+)\]')
NONTEST_RE = re.compile(r'// nontest')

interpreter_language = 'c'
interpreter_args = [join(REPO_DIR, 'clox', 'clox.exe')]
passed = 0
failed = 0
num_skipped = 0
expectations = 0

class Test:
    def __init__(self, path):
        self.path = path
        self.output = []
        self.compile_errors = set()
        self.runtime_error_line = 0
        self.runtime_error_message = None
        self.exit_code = 0
        self.failures = []

    def parse(self):
        global expectations

        line_num = 1
        with open(self.path, 'r') as file:
            for line in file:
                match = OUTPUT_EXPECT.search(line)
                if match:
                    self.output.append((match.group(1), line_num))
                    expectations += 1

                match = ERROR_EXPECT.search(line)
                if match:
                    self.compile_errors.add(
                        "[{0}] {1}".format(line_num, match.group(1)))

                    # If we expect a compile error, it should exit with EX_DATAERR.
                    self.exit_code = 65
                    expectations += 1

                match = ERROR_LINE_EXPECT.search(line)
                if match:
                    # The two interpreters are slightly different in terms of which
                    # cascaded errors may appear after an initial compile error because
                    # their panic mode recovery is a little different. To handle that,
                    # the tests can indicate if an error line should only appear for a
                    # certain interpreter.
                    language = match.group(2)
                    if not language or language == interpreter_language:
                        self.compile_errors.add("[{0}] {1}".format(
                            match.group(3), match.group(4)))

                        # If we expect a compile error, it should exit with EX_DATAERR.
                        self.exit_code = 65
                        expectations += 1

                match = RUNTIME_ERROR_EXPECT.search(line)
                if match:
                    self.runtime_error_line = line_num
                    self.runtime_error_message = match.group(1)
                    # If we expect a runtime error, it should exit with EX_SOFTWARE.
                    self.exit_code = 70
                    expectations += 1

                match = NONTEST_RE.search(line)
                if match:
                    # Not a test file at all, so ignore it.
                    return False

                line_num += 1

        # If we got here, it's a valid test.
        return True

    def run(self):
        # Invoke the interpreter and run the test.
        args = interpreter_args[:]
        args.append(self.path)
        proc = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE)

        out, err = proc.communicate()
        self.validate(proc.returncode, out, err)

    def validate(self, exit_code, out, err):
        if self.compile_errors and self.runtime_error_message:
            self.fail(
                "Test error: Cannot expect both compile and runtime errors.")
            return

        try:
            out = out.decode("utf-8").replace('\r\n', '\n')
            err = err.decode("utf-8").replace('\r\n', '\n')
        except:
            self.fail('Error decoding output.')

        error_lines = err.split('\n')

        # Validate that an expected runtime error occurred.
        if self.runtime_error_message:
            self.validate_runtime_error(error_lines)
        else:
            self.validate_compile_errors(error_lines)

        self.validate_exit_code(exit_code, error_lines)
        self.validate_output(out)

    def validate_runtime_error(self, error_lines):
        if len(error_lines) < 2:
            self.fail('Expected runtime error "{0}" and got none.',
                      self.runtime_error_message)
            return

        # Skip any compile errors. This can happen if there is a compile error in
        # a module loaded by the module being tested.
        line = 0
        while SYNTAX_ERROR_RE.search(error_lines[line]):
            line += 1

        if error_lines[line] != self.runtime_error_message:
            self.fail('Expected runtime error "{0}" and got:',
                      self.runtime_error_message)
            self.fail(error_lines[line])

        # Make sure the stack trace has the right line. Skip over any lines that
        # come from builtin libraries.
        match = False
        stack_lines = error_lines[line + 1:]
        for stack_line in stack_lines:
            match = STACK_TRACE_RE.search(stack_line)
            if match:
                break

        if not match:
            self.fail('Expected stack trace and got:')
            for stack_line in stack_lines:
                self.fail(stack_line)
        else:
            stack_line = int(match.group(1))
            if stack_line != self.runtime_error_line:
                self.fail('Expected runtime error on line {0} but was on line {1}.',
                          self.runtime_error_line, stack_line)

    def validate_compile_errors(self, error_lines):
        # Validate that every compile error was expected.
        found_errors = set()
        num_unexpected = 0
        for line in error_lines:
            match = SYNTAX_ERROR_RE.search(line)
            if match:
                error = "[{0}] {1}".format(match.group(1), match.group(2))
                if error in self.compile_errors:
                    found_errors.add(error)
                else:
                    if num_unexpected < 10:
                        self.fail('Unexpected error:')
                        self.fail(line)
                    num_unexpected += 1
            elif line != '':
                if num_unexpected < 10:
                    self.fail('Unexpected output on stderr:')
                    self.fail(line)
                num_unexpected += 1

        if num_unexpected > 10:
            self.fail('(truncated ' + str(num_unexpected - 10) + ' more...)')

        # Validate that every expected error occurred.
        for error in self.compile_errors - found_errors:
            self.fail('Missing expected error: {0}', error)

    def validate_exit_code(self, exit_code, error_lines):
        if exit_code == self.exit_code:
            return

        if len(error_lines) > 10:
            error_lines = error_lines[0:10]
            error_lines.append('(truncated...)')
        self.fail('Expected return code {0} and got {1}. Stderr:',
                  self.exit_code, exit_code)
        self.failures += error_lines

    def validate_output(self, out):
        # Remove the trailing last empty line.
        out_lines = out.split('\n')
        if out_lines[-1] == '':
            del out_lines[-1]

        index = 0
        for line in out_lines:
            if sys.version_info < (3, 0):
                line = line.encode('utf-8')

            if index >= len(self.output):
                self.fail('Got output "{0}" when none was expected.', line)
            elif self.output[index][0] != line:
                self.fail('Expected output "{0}" on line {1} and got "{2}".',
                          self.output[index][0], self.output[index][1], line)
            index += 1

        while index < len(self.output):
            self.fail('Missing expected output "{0}" on line {1}.',
                      self.output[index][0], self.output[index][1])
            index += 1

    def fail(self, message, *args):
        if args:
            message = message.format(*args)
        self.failures.append(message)


def walk(dir, callback):
    """
    Walks [dir], and executes [callback] on each file.
    """

    dir = abspath(dir)
    for file in listdir(dir):
        nfile = join(dir, file)
        if isdir(nfile):
            walk(nfile, callback)
        else:
            callback(nfile)


def run_script(path):
    if "benchmark" in path:
        return

    global passed
    global failed
    global num_skipped

    if (splitext(path)[1] != '.lox'):
        return

    # Make a nice short path relative to the working directory.

    # Normalize it to use "/" since, among other things, the interpreters expect
    # the argument to use that.
    path = relpath(path).replace("\\", "/")

    # Update the status line.
    term.print_line('Passed: {} Failed: {} Skipped: {} {}'.format(
        term.green(passed),
        term.red(failed),
        term.yellow(num_skipped),
        term.gray('({})'.format(path))))

    # Read the test and parse out the expectations.
    test = Test(path)

    if not test.parse():
        # It's a skipped or non-test file.
        return

    test.run()

    # Display the results.
    if len(test.failures) == 0:
        passed += 1
    else:
        failed += 1
        term.print_line(term.red('FAIL') + ': ' + path)
        print('')
        for failure in test.failures:
            print('      ' + term.pink(failure))
        print('')


def run_suite():
    global passed
    global failed
    global num_skipped
    global expectations

    passed = 0
    failed = 0
    num_skipped = 0
    expectations = 0

    walk(join(REPO_DIR, 'clox', 'test', 'lox'), run_script)
    term.print_line()
    
    print()

    if failed == 0:
        print('All {} tests passed ({} expectations).'.format(
            term.green(passed), str(expectations)))
    else:
        print('{} tests passed. {} tests failed.'.format(
            term.green(passed), term.red(failed)))

    return failed == 0


def main(argv):
    if len(argv) > 1:
        print('Usage: test.py')
        sys.exit(1)

    run_suite()


if __name__ == '__main__':
    main(sys.argv)
