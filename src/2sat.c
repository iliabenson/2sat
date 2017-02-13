#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// assumption: the values in the txt passed in are monotonically increasing starting from 1 to n, so 1, 2, 3,...,n where each one can be negative

static const int lineSize = 1024;

struct satEntry{
	int left;
	int right;
};

static void testPrint(int lines, struct satEntry* entries){
	int i;

	for(i = 0; i < lines; i++){
		printf("Left: %d, Right: %d\n", entries[i].left, entries[i].right);
	}
}

static void freePtr(void* ptr){
	free(ptr);
	ptr = NULL;
}

static char* Str_Tok_Multi(char* str, char const* delims){
	static char* src = NULL;
	char* p, *ret = 0;

	if(str != NULL){
		src = str;
	}

	if(src == NULL){
		return NULL;
	}

	if((p = strpbrk(src, delims)) != NULL){
		*p  = 0;
		ret = src;
		src = ++p;

	}else if (*src){
		ret = src;
		src = NULL;
	}

	return ret;
}

static inline int setToken(char* token, int* entry, int* max){
	if(token){
		*entry = atoi(token);
		if(abs(*entry) > *max){
			*max = abs(*entry);
		}

		return 1;
	}

	return -1;
}

static struct satEntry* readValues(int* lines, char* filename, int* max){
	struct satEntry* entries = NULL;
	int count = 0;
	FILE* fp = NULL;
	char buffer[lineSize];
	char* token = NULL;
	char delim[] = " ";

	if((fp = fopen(filename, "r")) == NULL){
		perror("ERROR: Could not open file!");
		exit(EXIT_FAILURE);
	}

	while(fgets(buffer, lineSize, fp) != NULL){
		count++;
	}

	*lines = count;
	count = 0;

	rewind(fp);

	entries = (struct satEntry*)malloc(count * sizeof(struct satEntry));

	while(fgets(buffer, lineSize, fp) != NULL){
		if(buffer[strlen(buffer) - 1] == '\n'){
			buffer[strlen(buffer) - 1] = '\0';
		}

		// Str_Tok_Multi is destructive
		token = Str_Tok_Multi(buffer, delim);
		if(setToken(token, &entries[count].left, max) < 0){
			perror("ERROR: Could not write left token!");
			exit(EXIT_FAILURE);
		}
		token = Str_Tok_Multi(NULL, delim);
		if(setToken(token, &entries[count].right, max) < 0){
			perror("ERROR: Could not write right token!");
			exit(EXIT_FAILURE);
		}

		count++;
	}

	return entries;
}

int main(int argc, char* argv[]){
	struct satEntry* entries = NULL;
	char* booleanValues = NULL;
	int lines = 0;
	int max = 0;

	if(argc <= 1){
		perror("ERROR: Must pass in file name of values!");
		exit(EXIT_FAILURE);
	}

	entries = readValues(&lines, argv[1], &max);
	// testPrint(lines, entries);
	printf("Number of tuples: %d, Max number: %d\n", lines, max);
	booleanValues = (char*)malloc(max * sizeof(char));
	memset(booleanValues, 'T', max);

	freePtr(entries);
	freePtr(booleanValues);
}
