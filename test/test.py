import random
import string
import subprocess
import os
from dataclasses import dataclass

# Constants
NUM_PAIRS_MAX = 256
NAME_LENGTH_MAX = 100
NUMBER_LENGTH_MAX = 100
C_PROGRAM_PATH = "C:/Prirodne_vedy_XX2/event_scheduler/VUT/IZP/projects/project1/build/Debug/TNINE.exe"

def random_string(length):
    return ''.join(random.choices(string.ascii_letters, k=length))

def generate(filename: str) -> list[str]:
    output_file_list: list[str] = []
    with open(filename, 'w') as f:
        for _ in range(random.randrange(1, NUM_PAIRS_MAX)):
            name = random_string(random.randrange(1, NAME_LENGTH_MAX))
            number = ''.join(random.choices(string.digits, k=random.randrange(1, NUMBER_LENGTH_MAX)))
            f.write(f"{name}\n{number}\n")
            output_file_list.append(name)
            output_file_list.append(number)
    return output_file_list

Error = int

def run_program(input_file, command) -> tuple[Error, str]:
    with open(input_file, 'r') as infile, open('output.txt', 'w') as outfile:
        result = subprocess.run(command, stdin=infile, stdout=outfile, stderr=subprocess.PIPE)
    return result.returncode, result.stderr.decode()

def check_output(expected_file):
    with open('output.txt', 'r') as f:
        output = f.read()
    
    with open(expected_file, 'r') as f:
        expected = f.read()
    
    return output == expected

@dataclass
class TestFootprint:
    _should_fail: tuple[bool, Error]
    extended_search: bool
    t9_number_enabled: bool

    @property
    def should_fail(self) -> bool:
        return self._should_fail[0]
    
    @property
    def failure_code(self) -> Error:
        return self._should_fail[1]

class Test:
    def __init__(self, footprint: TestFootprint, output_file: str):
        self.footprint = footprint
        self.commands = [C_PROGRAM_PATH] 
        self.generated_file = output_file
        self.generated_file_content = generate(output_file)
        if self.footprint.extended_search:
            self.commands.append("-s")
        if self.footprint.t9_number_enabled:
            if self.footprint.should_fail:
                # not really secure but... easy ;)
                self.commands.append(str(random.randint(294842, 2498428398942)))
            else:
                random_line_index = random.randrange(1, len(self.generated_file_content), 2)
                random_line = self.generated_file_content[random_line_index].strip()
                end = random.randint(1, len(random_line))
                begin = random.randint(0, end)
                self.commands.append(random_line[begin : end])

    def run_test(self) -> None:
        program_return = run_program(self.generated_file, self.commands)
        if program_return[0] == self.footprint.failure_code:
            self.test_successfull(program_return[0])
        else:
            self.test_fail(program_return[0])

    def test_successfull(self, program_return: int) -> None:
        print(f"Test [should_fail={self.footprint.should_fail}; command={self.commands}; returned/expected={self.footprint.failure_code}::{program_return}]\x1b[32m passed\x1b[0m")

    def test_fail(self, program_return: int) -> None:
        print(f"For arguments {self.commands}\n The following \x1b[31merror\x1b[0m ocurred: {program_return}; yet expected: {self.footprint.failure_code}\n")

if __name__ == '__main__':
    # test without t9number
    Test(
        TestFootprint(
            _should_fail=(False, 0,),
            extended_search=False,
            t9_number_enabled=False), "out/test_default_all.txt").run_test();

    # basic tests
    for i in range(100):
        Test(
            TestFootprint(
                _should_fail=(False, 0,),
                extended_search=False,
                t9_number_enabled=True), f"out/test_default{i}.txt").run_test();

    # test fault protection
    # invalid number of args
    t = Test(
        TestFootprint(
            _should_fail=(True, -1,),
            extended_search=False,
            t9_number_enabled=True), f"out/test_failure{1}.txt");
    t.commands.extend(["something", "that", "will", "cause", "havooooc"])
    # invalid number arg
    t = Test(
        TestFootprint(
            _should_fail=(True, -1,),
            extended_search=False,
            t9_number_enabled=True), f"out/test_failure{2}.txt");
    t.commands[1] = "23dfus23" # argument is not a number
    # file size mismatch
    # TODO: WHAT IF ONE OF THE LINES IS NOT A NUMBER OR A NAME ??
    t = Test(
        TestFootprint(
            _should_fail=(True, -1,),
            extended_search=False,
            t9_number_enabled=True), f"out/test_failure{5}.txt");
    with open(f"test_failure{5}.txt", "w") as file:
        file.writelines(t.generated_file_content[:-1]) # skip last line

    # test with extended search
    Test(
        TestFootprint(
            _should_fail=(False, 0,),
            extended_search=True,
            t9_number_enabled=True), "out/test_ex_search.txt").run_test();