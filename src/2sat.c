#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static const int lineSize = 1024;

struct satVariable{
	int64_t label; // variable label, and index into variableValues array
	char value; // it's boolean value
	int64_t* indexes; // locations where it appears in terminals array, even index means it is the left variable in the clause, odd means right
	int64_t count; // number of times it appears in terminals array, also length of indexes array
	int64_t weightedCount; // how many it appears in clauses where variables have not been assigned yet
	int64_t* negIndexes; // same but negated
	int64_t negCount; // same but negated
	int64_t negWeightedCount; // same but negated
	char evaluated;
};

static void freePtr(void* ptr){
	free(ptr);
	ptr = NULL;
}

static void testPrint(int64_t* terminals, int64_t numTerminals, char* variableValues, int64_t numVariableValues, struct satVariable* variableTable, int64_t numVariableTable){
	int64_t i, j;

	printf("Total number of terminals: %lld\n", numTerminals);
	printf("Total number of clauses: %lld\n", numTerminals / 2);
	printf("Number of variables: %lld\n", numVariableValues);
	printf("Number of used variables: %lld\n", numVariableTable);

	for(i = 0; i < numTerminals; i++){
		printf("%lld ", terminals[i]);

		if(i % 2 != 0){
			printf("\n");
		}
	}

	for(i = 0; i <= numVariableValues; i++){
		printf("%c ", variableValues[i]);
	}

	printf("\n\n");

	for(i = 0; i < numVariableTable; i++){
		printf("Variable: %lld, Value: %c, Positive Count: %lld/%lld, Negated Count: %lld/%lld\n", variableTable[i].label, variableTable[i].value, variableTable[i].count, variableTable[i].weightedCount, variableTable[i].negCount, variableTable[i].negWeightedCount);
		printf("Evaluated: %c\n", variableTable[i].evaluated);
		printf("Positive Index locations in terminals array: ");
		for(j = 0; j < variableTable[i].count; j++){
			printf("%lld ", variableTable[i].indexes[j]);
		}
		printf("\n");
		printf("Negated Index locations in terminals array: ");
		for(j = 0; j < variableTable[i].negCount; j++){
			printf("%lld ", variableTable[i].negIndexes[j]);
		}
		printf("\n\n");
	}
}

// Destructive function, quick and dirty mutli delim string tokenizer
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

static void setToken(char* token, int64_t* terminals, int64_t position, int64_t* numVariableValues){
	if(token){
		terminals[position] = atoi(token);

		if(abs(terminals[position]) > *numVariableValues){
			*numVariableValues = abs(terminals[position]);
		}
	}
	else{
		fprintf(stderr,"Could not write a token, %s, exiting", token);
		exit(EXIT_FAILURE);
	}
}

static int64_t* readClauses(char* filename, int64_t* numTerminals, int64_t* numVariableValues){
	int64_t* terminals = NULL;
	int64_t count = 0;
	FILE* fp = NULL;
	char buffer[lineSize];
	char delim[] = " ";

	if((fp = fopen(filename, "r")) == NULL){
		perror("ERROR: Could not open file!");
		exit(EXIT_FAILURE);
	}

	while(fgets(buffer, lineSize, fp) != NULL){
		count++;
	}

	*numTerminals = (2 * count); // each line has 2 variables (could be non-unique) so call that number of terminals

	rewind(fp);

	terminals = (int64_t*)malloc(*numTerminals * sizeof(int64_t));
	count = 0;

	while(fgets(buffer, lineSize, fp) != NULL){
		if(buffer[strlen(buffer) - 1] == '\n'){
			buffer[strlen(buffer) - 1] = '\0';
		}

		// Str_Tok_Multi is destructive
		setToken(Str_Tok_Multi(buffer, delim), terminals, count, numVariableValues);
		count++;

		setToken(Str_Tok_Multi(NULL, delim), terminals, count, numVariableValues);
		count++;
	}

	fclose(fp);

	return terminals;
}

static char* findUsedVariables(int64_t* terminals, int64_t numTerminals , int64_t numVariableValues, int64_t* numVariableTable){
	int64_t i;

	char* variableValues = (char*)malloc((numVariableValues + 1) * sizeof(char));
	memset(variableValues, 'F', (numVariableValues + 1) * sizeof(char));
	variableValues[0] = 'X';

	for(i = 0; i < numTerminals; i++){
		variableValues[abs(terminals[i])] = 'T'; // set all the present variables to T
	}

	for(i = 1; i <= numVariableValues; i++){
		if(variableValues[i] == 'T'){
			++*numVariableTable;
		}
	}

	return variableValues;
}

static struct satVariable* makeVariableTable(int64_t* terminals, int64_t numTerminals, int64_t numVariableTable){
	int64_t i, j;
	struct satVariable* variableTable = NULL;

	variableTable = (struct satVariable*)calloc(numVariableTable, numVariableTable * sizeof(struct satVariable));

	// @NOTE: slow but i only need to do this once
	for(i = 0; i < numTerminals; i++){
		for(j = 0; j < numVariableTable; j++){
			if(variableTable[j].label == abs(terminals[i])){
				if(terminals[i] > 0){
					variableTable[j].indexes[variableTable[j].count] = i;
					variableTable[j].count++;
					variableTable[j].weightedCount++;
				}
				else{
					variableTable[j].negIndexes[variableTable[j].negCount] = i;
					variableTable[j].negCount++;
					variableTable[j].negWeightedCount++;
				}

				break; // found existing variable and updated it, break out of inner loop
			}
			else if(variableTable[j].label == 0){
				variableTable[j].label = abs(terminals[i]);
				variableTable[j].value = 'T';
				variableTable[j].indexes = (int64_t*)malloc(numTerminals * sizeof(int64_t));
				variableTable[j].count = 0;
				variableTable[j].weightedCount = 0;
				variableTable[j].negIndexes = (int64_t*)malloc(numTerminals * sizeof(int64_t));
				variableTable[j].negCount = 0;
				variableTable[j].evaluated = 'F';
				variableTable[j].negWeightedCount = 0;

				if(terminals[i] > 0){
					variableTable[j].indexes[variableTable[j].count] = i;
					variableTable[j].count++;
					variableTable[j].weightedCount++;
				}
				else{
					variableTable[j].negIndexes[variableTable[j].negCount] = i;
					variableTable[j].negCount++;
					variableTable[j].negWeightedCount++;
				}

				break; // made a new variable and updated it, break out of inner loop
			}
		}
	}

	// @TODO: shrink indexes and negIndexes

	return variableTable;
}

static int64_t* makeReverseLookUpTable(int64_t* reverseLookUpTable, int64_t numVariableValues, struct satVariable* variableTable, int64_t numVariableTable){
	int64_t i;

	reverseLookUpTable = (int64_t*)malloc((numVariableValues + 1) * sizeof(int64_t));
	memset(reverseLookUpTable, -1, (numVariableValues + 1) * sizeof(char));

	for(i = 0; i < numVariableTable; i++){
		reverseLookUpTable[variableTable[i].label] = i;
	}

	return reverseLookUpTable;
}

static inline int64_t findVariableInTable(struct satVariable* variableTable, int64_t numVariableTable, int64_t target){
	// use reverse look up table, make inline functions
}

static void greedyAnalysis(int64_t* terminals, int64_t numTerminals, char* variableValues, int64_t numVariableValues, struct satVariable* variableTable, int64_t numVariableTable){
	int64_t count = 0;
	int64_t totalSat = 0;
	int64_t i, current, largest;

	while(count < numTerminals){
		largest = 0;
		current = -1;
		for(i = 0; i < numVariableTable; i++){
			if(variableTable[i].evaluation == 'F' && (variableTable[i].count > largest || variableTable[i].negCount > largest)){
				current = i;
				if(variableTable[i].count > variableTable[i].negCount){
					largest = variableTable[i].count;
				}
				else{
					largest = variableTable[i].negCount;
				}
			}
		}

		count += variableTable[i].weightedCount;
	}
}

int main(int argc, char* argv[]){
	int64_t i;
	int64_t* terminals = NULL; // all the terminals as they appear in the file, a terminal is a variable in a clause so (A v A) has 2 terminals but 1 variable
	int64_t numTerminals = 0; // total number if variables (with repetition) referred to as terminals
	char* variableValues = NULL; // this will contain the answer at the end, size has +1 for direct index mapping purposes
	int64_t* reverseLookUpTable = NULL; // this table maps the (index) value of the variable values to the (value) index of it in the variableTable, same size as variableValues table, +1 for direct mapping
	int64_t numVariableValues = 0; // number of variables in the files (used and unused)
	struct satVariable* variableTable = NULL; // used for evaluation of each used variable, contains relevant info for that variable
	int64_t numVariableTable = 0; // number of used variables in the file only

	if(argc <= 1){
		perror("ERROR: Must pass in file name of values!");
		exit(EXIT_FAILURE);
	}

	terminals = readClauses(argv[1], &numTerminals, &numVariableValues);
	variableValues = findUsedVariables(terminals, numTerminals, numVariableValues, &numVariableTable);
	variableTable = makeVariableTable(terminals, numTerminals, numVariableTable);
	reverseLookUpTable = makeReverseLookUpTable(reverseLookUpTable, numVariableValues, variableTable, numVariableTable);

	testPrint(terminals, numTerminals, variableValues, numVariableValues, variableTable, numVariableTable);

	freePtr(terminals);
	freePtr(variableValues);
	for(i = 0; i < numVariableTable; i++){
		freePtr(variableTable[i].indexes);
		freePtr(variableTable[i].negIndexes);
	}
	freePtr(variableTable);
}
