import random
import string
import subprocess
import os
from enum import Enum
from os import path
from dataclasses import dataclass
from abc import ABC, abstractmethod
from time import time

# Constants
NUM_PAIRS_MAX = 100
NAME_LENGTH_MAX = 100
NUMBER_LENGTH_MAX = 100

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

def run_program(_, command) -> tuple[Error, str]:
    with open('output.txt', 'w') as outfile:
        print(f"Run program args: {command}")
        result = subprocess.run(command, stdout=outfile, stderr=subprocess.PIPE, shell=True)
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

    def set_failure_code(self, code) -> None:
        self._should_fail = (self._should_fail[0], code,)


class Test(ABC):
    @abstractmethod
    def __init__(self):
        raise Exception("Not implemented!")

    @abstractmethod
    def run_test(self):
        raise Exception("Not implemented!")

class DefaultTest(Test):
    def __init__(self, footprint: TestFootprint, output_file: str, program_path: str):
        self.footprint = footprint
        if self.footprint.should_fail:
            self.footprint.set_failure_code(self.footprint.failure_code + (1 << 32))
        self.commands = [program_path] 
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
        self.commands.append(f"<{self.generated_file}")

    def run_test(self) -> None:
        program_return = run_program(self.generated_file, self.commands)
        if program_return[0] == self.footprint.failure_code:
            self.__test_successfull(program_return[0])
        else:
            self.__test_fail(program_return[0])

    def __test_successfull(self, program_return: int) -> None:
        print(f"Test [should_fail={self.footprint.should_fail}; command={self.commands}; returned/expected={self.footprint.failure_code}::{program_return}]\x1b[32m passed\x1b[0m\n")

    def __test_fail(self, program_return: int) -> None:
        print(f"For arguments {self.commands}\n The following \x1b[31merror\x1b[0m ocurred: {program_return}; yet expected: {self.footprint.failure_code}\n")

class TimedTest(Test):
    def __init__(self, footprint: TestFootprint, output_file: str, program_path: str):
        self.footprint = footprint
        self.commands = [program_path] 
        self.generated_file = output_file
        self.generated_file_content = generate(output_file)
        if self.footprint.extended_search:
            self.commands.append("-s")
        if self.footprint.t9_number_enabled:
            random_line_index = random.randrange(1, len(self.generated_file_content), 2)
            random_line = self.generated_file_content[random_line_index].strip()
            end = random.randint(1, len(random_line))
            begin = random.randint(0, end)
            self.commands.append(random_line[begin : end])
        self.commands.append(f"<{self.generated_file}")

    def run_test(self) -> None:
        t1 = time()
        program_return = run_program(self.generated_file, self.commands)
        if program_return[0] != 0:
            raise Exception(f"The program run failed with exit code: {program_return[0]}.\nMake sure that the program works properly befor running the timed test!")
        t2 = time()
        print(f"Test [command={self.commands}]\x1b[33m {(t2 - t1) * 1000}ms\x1b[0m")

class Program:
    class Program_Run_Type(Enum):
        DEFAULT = ord('0')
        TIMER = ord('1')

    @staticmethod
    def run():
        program_path = input("Enter program path: ")
        type = ord(input("Enter program run type:\nDefault test[0]\nTimer test[1]")[0])
        match type:
            case Program.Program_Run_Type.DEFAULT.value:
                Program.__def_run(program_path)
            case Program.Program_Run_Type.TIMER.value:
                Program.__timer_run()
            case _:
                raise Exception("Invalid program type!")

    def __def_run(program_path: str):
        if not path.isdir("out"):
            os.mkdir("out")

        # test without t9number
        DefaultTest(
            TestFootprint(
                _should_fail=(False, 0,),
                extended_search=False,
                t9_number_enabled=False), "out/test_default_all.txt",
                program_path=program_path).run_test();

        # basic tests
        for i in range(100):
            DefaultTest(
                TestFootprint(
                    _should_fail=(False, 0,),
                    extended_search=False,
                    t9_number_enabled=True), f"out/test_default{i}.txt",
                    program_path=program_path).run_test();

        # test fault protection

        # invalid number of args
        t = DefaultTest(
            TestFootprint(
                _should_fail=(True, -1,),
                extended_search=False,
                t9_number_enabled=True), f"out/test_failure{1}.txt",
                program_path=program_path);
        t.commands.extend(["something", "that", "will", "cause", "havooooc"])
        t.run_test()

        # invalid number arg
        t = DefaultTest(
            TestFootprint(
                _should_fail=(True, -2,),
                extended_search=False,
                t9_number_enabled=True), f"out/test_failure{2}.txt",
                program_path=program_path);
        t.commands[1] = "23dfus23" # argument is not a number
        t.run_test()

        # invalid number arg length
        t = DefaultTest(
            TestFootprint(
                _should_fail=(True, -3,),
                extended_search=False,
                t9_number_enabled=True), f"out/test_failure{3}.txt",
                program_path=program_path);
        t.commands[1] += "2" * (NUMBER_LENGTH_MAX)
        t.run_test()

        # invalid number
        t = DefaultTest(
            TestFootprint(
                _should_fail=(True, -4,),
                extended_search=False,
                t9_number_enabled=True), f"out/test_failure{4}.txt",
                program_path=program_path);
        with open(f"out/test_failure{4}.txt", "r+") as file:
            random_number_idx = random.randrange(1, len(t.generated_file_content), 2)
            for index, line in enumerate(file.readlines()):
                if random_number_idx == index: 
                    temp_line: str = ""
                    rand_idx = random.randrange(0, len(line) - 1)
                    temp_line += line[:rand_idx]
                    temp_line += 'X'
                    temp_line += line[rand_idx+1:]
                    file.write(temp_line)
                    continue
                file.write(line)
        t.run_test()

        # file too large
        t = DefaultTest(
            TestFootprint(
                _should_fail=(True, -5,),
                extended_search=False,
                t9_number_enabled=True), f"out/test_failure{5}.txt",
                program_path=program_path);
        with open(f"out/test_failure{5}.txt", "r+") as file:
            for index, line in enumerate(file.readlines()):
                file.write(line)
            file.writelines(['0' * 10 for _ in range(NUM_PAIRS_MAX)])
        t.run_test()

        # file size mismatch
        # TODO: WHAT IF ONE OF THE LINES IS NOT A NUMBER OR A NAME ??
        t = DefaultTest(
            TestFootprint(
                _should_fail=(True, -6,),
                extended_search=False,
                t9_number_enabled=True), f"out/test_failure{6}.txt",
                program_path=program_path);
        with open(f"test_failure{6}.txt", "w+") as file:
            file.writelines([x + "\n" for x in t.generated_file_content[:-1]]) # skip last line
        t.run_test()

        # line too large
        t = DefaultTest(
            TestFootprint(
                _should_fail=(True, -7,),
                extended_search=False,
                t9_number_enabled=True), f"out/test_failure{7}.txt",
                program_path=program_path);
        with open(f"out/test_failure{7}.txt", "r+") as file:
            random_number_idx = random.randrange(1, len(t.generated_file_content), 2)
            for index, line in enumerate(file.readlines()):
                if random_number_idx == index: 
                    if random_number_idx % 2 == 0:
                        line += "X" * (NUMBER_LENGTH_MAX)
                    else:
                        line += "0" * (NUMBER_LENGTH_MAX) # illegal character
                file.write(line)
        t.run_test()

        # test with extended search
        DefaultTest(
            TestFootprint(
                _should_fail=(False, 0,),
                extended_search=True,
                t9_number_enabled=True), "out/test_ex_search.txt",
                program_path=program_path).run_test();
        t.run_test()

    def __timer_run(program_path: str):
        pass

if __name__ == '__main__':
    Program.run()