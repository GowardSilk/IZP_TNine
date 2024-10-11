#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// note: this is copied here because we do not have a common header (for now) for the tnine project, once we divide that project
// accordingly this will be erased and included from the header
#define MAX_LINE_WIDTH 100
#define MAX_STR_LEN MAX_LINE_WIDTH + 1
#define MAX_PHONE_ITEMS 256
#define MAX_PHONE_ITEMS_FILE_SIZE (MAX_LINE_WIDTH + sizeof('\0')) * MAX_PHONE_ITEMS

#define random_reset() srand(time(NULL))
#define random() rand()

static char* alloc_string(size_t* string_size) {
	*string_size = (size_t)random() % MAX_STR_LEN;
	return calloc(*string_size, sizeof(char));
}

char* generate_string_name(size_t* string_size) {
	char* string = alloc_string(string_size);
	for (size_t i = 0; i < *string_size - 1; i++) {
		if (random() % 2 == 0) {
			string[i] = 'a' + random() % 21;
		}
		else {
			string[i] = 'A' + random() % 21;
		}
	}
	string[*string_size - 1] = '\n';
	return string;
}

char* generate_string_number(size_t* string_size) {
	char* string = alloc_string(string_size);
	for (size_t i = 0; i < *string_size - 1; i++) {
		string[i] = '0' + random() % 9;
	}
	string[*string_size - 1] = '\n';
	return string;
}

int main(void) {

	FILE* file = fopen("text_file.txt", "w+");
	if (file == NULL) {
		perror("File could not be opened: ");
		return 1;
	}
	char* name = NULL, *number = NULL;
	size_t name_size = 0, number_size = 0;
	random_reset();
	for (size_t i = 0; i < random() % 100; i++) {
		// note: we do not really care about the performance of this...
		name = generate_string_name(&name_size);
		number = generate_string_number(&number_size);
		fwrite(name, name_size, 1, file);
		fwrite(number, number_size, 1, file);
		printf("%s", name);
		printf("%s", number);
		free(name); name = NULL;
		free(number); number = NULL;
	}
	fclose(file);

	return 0;
}
