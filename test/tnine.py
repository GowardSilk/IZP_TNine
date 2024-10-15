T9_MAP = {
    '2': 'abc', '3': 'def', '4': 'ghi', '5': 'jkl', 
    '6': 'mno', '7': 'pqrs', '8': 'tuv', '9': 'wxyz'
}

def contains_substring(main_str, substring) -> bool:
    return substring in main_str

def t9_to_letters(input_str) -> list[str]:
    possible_combinations = ['']
    
    for digit in input_str:
        if digit in T9_MAP:
            letters = T9_MAP[digit]
            new_combinations = []
            for combination in possible_combinations:
                for letter in letters:
                    new_combinations.append(combination + letter)
            possible_combinations = new_combinations
    
    return possible_combinations

def find_matches(pairs, input_str) -> list[str]:
    if input_str == "":
        return pairs

    matches = []
    is_number = input_str.isdigit()

    for i in range(0, len(pairs), 2):
        print("kokto")
        name = pairs[i]
        number = pairs[i + 1]

        if is_number:
            if contains_substring(number, input_str):
                matches.extend([name, number])

            possible_names = t9_to_letters(input_str)
            for possible_name in possible_names:
                if contains_substring(name.lower(), possible_name):
                    matches.extend([name, number])

        else:
            if contains_substring(name.lower(), input_str.lower()):
                matches.extend([name, number])

    return matches