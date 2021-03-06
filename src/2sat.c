#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

static const int lineSize = 1024;

struct satVariable{
	int64_t label; // variable label, and index into variableValues array
	char value; // it's boolean value
	int64_t size;
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

// @TODO: comment this out before submission
// static void testPrint(int64_t* terminals, int64_t numTerminals, char* variableValues, int64_t numVariableValues, struct satVariable* variableTable, int64_t numVariableTable){
// 	int64_t i, j;

// 	printf("Total number of terminals: %lld\n", numTerminals);
// 	printf("Total number of clauses: %lld\n", numTerminals / 2);
// 	printf("Number of variables: %lld\n", numVariableValues);
// 	printf("Number of used variables: %lld\n", numVariableTable);

// 	for(i = 0; i < numTerminals; i++){
// 		printf("%lld ", terminals[i]);

// 		if(i % 2 != 0){
// 			printf("\n");
// 		}
// 	}

// 	for(i = 0; i <= numVariableValues; i++){
// 		printf("%c ", variableValues[i]);
// 	}

// 	printf("\n\n");

// 	for(i = 0; i < numVariableTable; i++){
// 		printf("Variable: %lld, Value: %c, Positive Count: %lld/%lld, Negated Count: %lld/%lld\n", variableTable[i].label, variableTable[i].value, variableTable[i].count, variableTable[i].weightedCount, variableTable[i].negCount, variableTable[i].negWeightedCount);
// 		printf("Evaluated: %c\n", variableTable[i].evaluated);
// 		printf("Positive Index locations in terminals array: ");
// 		for(j = 0; j < variableTable[i].count; j++){
// 			printf("%lld ", variableTable[i].indexes[j]);
// 		}
// 		printf("\n");
// 		printf("Negated Index locations in terminals array: ");
// 		for(j = 0; j < variableTable[i].negCount; j++){
// 			printf("%lld ", variableTable[i].negIndexes[j]);
// 		}
// 		printf("\n\n");
// 	}
// }

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
	memset(variableValues, 'X', (numVariableValues + 1) * sizeof(char)); // is value for this variable is not used in clauses, and therefor does not matter, however, for greedy if the variables sibling already sat the clause than it's value for that clause is 0, a variable can be used and have 0 weight, thus it is also represented by an X, later updated to F for purposes of local search.
	variableValues[0] = 'S'; // P is just a place holder, should never print except for testPrint

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

static void growVarTableIndexArray(struct satVariable* variable, int64_t* size){
	*size = *size * 2;
	variable->indexes = (int64_t*)realloc(variable->indexes,*size * sizeof(int64_t));
	variable->negIndexes = (int64_t*)realloc(variable->negIndexes,*size * sizeof(int64_t));
}

static struct satVariable* makeVariableTable(int64_t* terminals, int64_t numTerminals, int64_t numVariableTable){
	int64_t i, j;
	struct satVariable* variableTable = NULL;

	variableTable = (struct satVariable*)malloc(numVariableTable * sizeof(struct satVariable));

	for(i = 0; i < numVariableTable; i++){
		variableTable[i].label = -1;
	}

	// @NOTE: slow but i only need to do this once
	for(i = 0; i < numTerminals; i++){
		for(j = 0; j < numVariableTable; j++){

			if(variableTable[j].label == abs(terminals[i])){
				if(terminals[i] > 0){
					if(variableTable[j].count == variableTable[j].size){
						growVarTableIndexArray(&variableTable[j], &variableTable[j].size);
					}

					variableTable[j].indexes[variableTable[j].count] = i;
					variableTable[j].count++;
					variableTable[j].weightedCount++;
				}
				else{
					if(variableTable[j].negCount == variableTable[j].size){
						growVarTableIndexArray(&variableTable[j], &variableTable[j].size);
					}

					variableTable[j].negIndexes[variableTable[j].negCount] = i;
					variableTable[j].negCount++;
					variableTable[j].negWeightedCount++;
				}

				break; // found existing variable and updated it, break out of inner loop
			}
			else if(variableTable[j].label == -1){
				variableTable[j].label = abs(terminals[i]);
				variableTable[j].value = 'T';
				variableTable[j].size = 100;
				variableTable[j].indexes = (int64_t*)malloc(variableTable[j].size * sizeof(int64_t));
				variableTable[j].count = 0;
				variableTable[j].weightedCount = 0;
				variableTable[j].negIndexes = (int64_t*)malloc(variableTable[j].size * sizeof(int64_t));
				variableTable[j].negCount = 0;
				variableTable[j].negWeightedCount = 0;
				variableTable[j].evaluated = 'F';

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

	// Shrink the indexes arrays
	for(j = 0; j < numVariableTable; j++){
		variableTable[j].indexes = (int64_t*)realloc(variableTable[j].indexes, variableTable[j].count * sizeof(int64_t));
		variableTable[j].negIndexes = (int64_t*)realloc(variableTable[j].negIndexes, variableTable[j].negCount * sizeof(int64_t));
	}

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

static inline void decrementSibWeight(int64_t instance, int64_t* terminals, struct satVariable* variableTable, int64_t* reverseLookUpTable){
	int64_t sibling;
	int64_t variabltTableIndex;

	// determine if instance is left variable or right variable in the clause
	if(instance % 2 == 0){
		sibling = instance + 1;
	}
	else{
		sibling = instance - 1;
	}
	// get sibling and look up it's index in reverseLookUpTable
	// use the result to decrement its pos or neg weight in variableTable
	if(terminals[sibling] > 0){
		variabltTableIndex = reverseLookUpTable[terminals[sibling]];
		variableTable[variabltTableIndex].weightedCount--;
	}
	else{
		variabltTableIndex = reverseLookUpTable[abs(terminals[sibling])];
		variableTable[variabltTableIndex].negWeightedCount--;
	}
}

static void greedyAnalysis(int64_t* terminals, int64_t numTerminals, struct satVariable* variableTable, int64_t numVariableTable, int64_t* reverseLookUpTable){
	int64_t count = 0;
	int64_t dominant = 0;
	int64_t i, current, largest;

	while(count < numTerminals){
		largest = 0;
		current = -1;
		for(i = 0; i < numVariableTable; i++){
			if(variableTable[i].evaluated == 'F' && (variableTable[i].weightedCount >= largest || variableTable[i].negWeightedCount >= largest)){
				current = i;
				if(variableTable[i].weightedCount > variableTable[i].negWeightedCount){
					largest = variableTable[i].weightedCount;
					dominant = 1;
				}
				else{
					largest = variableTable[i].negWeightedCount;
					dominant = -1;
				}
			}
		}

		// found current largest
		// assign value T/F
		if(dominant > 0){
			variableTable[current].value = 'T';
		}
		else{
			variableTable[current].value = 'F';
		}
		// set evaluated to T
		variableTable[current].evaluated = 'T';

		// if largest is 0, it means i am examining an item that has no weight, thus its value does not matter but i still need to evaluate it
		// this is probably due to it's sibling satifying the clauses, thus their weights do not need to be decremented as they are already at 0
		if(largest != 0){
			// go through currents indexes and decrease all of it's siblings weights by 1 since those clauses have been satisfied and siblings have no weight in them
			// over decrementation is fine, only happens when variable has already been evaluated and set to -1 weight previously, all other decrementations are valid
			for(i = 0; i < variableTable[current].count; i++){
				decrementSibWeight(variableTable[current].indexes[i], terminals, variableTable, reverseLookUpTable);
			}
			for(i = 0; i < variableTable[current].negCount; i++){
				decrementSibWeight(variableTable[current].negIndexes[i], terminals, variableTable, reverseLookUpTable);
			}
		}

		// set weights to -1 since it has none anymore, weight will continue to decrease but as long as it is in the negatives after being evaluated all is fine
		variableTable[current].weightedCount = -1;
		variableTable[current].negWeightedCount = -1;

		// increment count by currents total pos and neg counts
		count = count + variableTable[current].count + variableTable[current].negCount;
	}
}

static inline void updateVariableValues(char* variableValues, struct satVariable* variableTable, int64_t numVariableTable){
	int64_t i;

	for(i = 0; i < numVariableTable; i++){
		variableValues[variableTable[i].label] = variableTable[i].value;

	}
}

static inline void printValues(char* variableValues, int64_t numVariableValues){
	int64_t i;

	for(i = 1; i <= numVariableValues; i++){
		printf("%c", variableValues[i]);
	}

	printf("\n");
}

static inline int64_t evaluateAssignments(int64_t* terminals, int64_t numTerminals, char* variableValues){
	int64_t i;
	int64_t count = 0;

	for(i = 0; i < numTerminals - 1; i += 2){
		if(terminals[i] > 0 && terminals[i + 1] > 0){
			if(variableValues[abs(terminals[i])] == 'T' || variableValues[abs(terminals[i + 1])] == 'T'){
				count++;
			}
		}
		else if(terminals[i] > 0 && terminals[i + 1] < 0){
			if(variableValues[abs(terminals[i])] == 'T' || variableValues[abs(terminals[i + 1])] == 'F'){
				count++;
			}
		}
		else if(terminals[i] < 0 && terminals[i + 1] > 0){
			if(variableValues[abs(terminals[i])] == 'F' || variableValues[abs(terminals[i + 1])] == 'T'){
				count++;
			}
		}
		else if(terminals[i] < 0 && terminals[i + 1] < 0){
			if(variableValues[abs(terminals[i])] == 'F' || variableValues[abs(terminals[i + 1])] == 'F'){
				count++;
			}
		}
	}

	return count;
}

static int64_t randint(int64_t n) {
	if((n - 1) == RAND_MAX){
		return rand();
	}else{

	int64_t end = RAND_MAX / n;
	assert (end > 0L);
	end *= n;

	int64_t r;
	while ((r = rand()) >= end);

	return r % n;
	}
}

static inline int64_t selectRandVariable(int64_t* unSatTerminals, int64_t numUnSatTerminals, int64_t* reverseLookUpTable){
	int64_t randIndex = randint(numUnSatTerminals);
	int64_t selectedVariableIndex = -1;

	// decide if i want left or right sibling
	// if even i want left sibling, otherwise choose right
	if(randint(numUnSatTerminals) % 2 == 0){
		if(randIndex % 2 == 0){
			selectedVariableIndex = reverseLookUpTable[abs(unSatTerminals[randIndex])];
		}
		else{
			selectedVariableIndex = reverseLookUpTable[abs(unSatTerminals[randIndex - 1])];
		}
	}
	else{
		if(randIndex % 2 == 0){
			selectedVariableIndex = reverseLookUpTable[abs(unSatTerminals[randIndex + 1])];
		}
		else{
			selectedVariableIndex = reverseLookUpTable[abs(unSatTerminals[randIndex])];
		}
	}

	return selectedVariableIndex; // in variableTable
}

static inline void flipVariableValue(struct satVariable* variable){
	if(variable->value == 'T'){
		variable->value = 'F';
	}
	else{
		variable->value = 'T';
	}
}

static int64_t* makeUnSatTerminals(int64_t* numUnSatTerminals, int64_t* terminals, int64_t numTerminals, char* variableValues){
	int64_t i;
	int64_t clauses = 0;
	int64_t* unSatTerminals = NULL;

	for(i = 0; i < numTerminals - 1; i += 2){
		if(terminals[i] > 0 && terminals[i + 1] > 0){
			if(variableValues[abs(terminals[i])] == 'F' || variableValues[abs(terminals[i + 1])] == 'F'){
				clauses++;
			}
		}
		else if(terminals[i] > 0 && terminals[i + 1] < 0){
			if(variableValues[abs(terminals[i])] == 'F' || variableValues[abs(terminals[i + 1])] == 'T'){
				clauses++;
			}
		}
		else if(terminals[i] < 0 && terminals[i + 1] > 0){
			if(variableValues[abs(terminals[i])] == 'T' || variableValues[abs(terminals[i + 1])] == 'F'){
				clauses++;
			}
		}
		else if(terminals[i] < 0 && terminals[i + 1] < 0){
			if(variableValues[abs(terminals[i])] == 'T' || variableValues[abs(terminals[i + 1])] == 'T'){
				clauses++;
			}
		}
	}

	*numUnSatTerminals = clauses * 2; // i just counted clauses
	unSatTerminals = (int64_t*)malloc((clauses * 2) * sizeof(int64_t));
	clauses = 0;

	for(i = 0; i < numTerminals - 1; i += 2){
		if(terminals[i] > 0 && terminals[i + 1] > 0){
			if(variableValues[abs(terminals[i])] == 'F' || variableValues[abs(terminals[i + 1])] == 'F'){
				unSatTerminals[clauses] = terminals[i];
				unSatTerminals[clauses + 1] = terminals[i + 1];
				clauses += 2;
			}
		}
		else if(terminals[i] > 0 && terminals[i + 1] < 0){
			if(variableValues[abs(terminals[i])] == 'F' || variableValues[abs(terminals[i + 1])] == 'T'){
				unSatTerminals[clauses] = terminals[i];
				unSatTerminals[clauses + 1] = terminals[i + 1];
				clauses += 2;
			}
		}
		else if(terminals[i] < 0 && terminals[i + 1] > 0){
			if(variableValues[abs(terminals[i])] == 'T' || variableValues[abs(terminals[i + 1])] == 'F'){
				unSatTerminals[clauses] = terminals[i];
				unSatTerminals[clauses + 1] = terminals[i + 1];
				clauses += 2;
			}
		}
		else if(terminals[i] < 0 && terminals[i + 1] < 0){
			if(variableValues[abs(terminals[i])] == 'T' || variableValues[abs(terminals[i + 1])] == 'T'){
				unSatTerminals[clauses] = terminals[i];
				unSatTerminals[clauses + 1] = terminals[i + 1];
				clauses += 2;
			}
		}
	}

	return unSatTerminals;
}

static int64_t* removeClausesWithVariable(int64_t variableLabel, int64_t* unSatTerminals, int64_t* numUnSatTerminals){
	int64_t i;
	int64_t size = 0;
	int64_t* newArray = (int64_t*)malloc(*numUnSatTerminals * sizeof(int64_t));

	for(i = 0; i < *numUnSatTerminals; i++){
		if(abs(unSatTerminals[i]) == variableLabel){
			if(i % 2 == 0){
				i++; // skip i, i + 1 skipped in loop increment
			}
			else{
				size--; // reverse counter 1 to overwrite i - 1, i skipped in loop increment
			}
		}
		else{
			// add the terminal
			newArray[size] = unSatTerminals[i];
			size++;
		}
	}

	freePtr(unSatTerminals);
	*numUnSatTerminals = size;
	newArray = (int64_t*)realloc(newArray, size * sizeof(int64_t));
	return newArray;
}

static void localSearch(int64_t maxTries, int64_t* result, int64_t* terminals, int64_t numTerminals, struct satVariable* variableTable, int64_t numVariableTable, char* variableValues, int64_t* reverseLookUpTable){
	int64_t i;
	int64_t lastVariableFlippedIndex = -1;
	int64_t currentResult = 0;
	int64_t* unSatTerminals = NULL;
	int64_t numUnSatTerminals = 0;

	// NOTE: maybe modify this so that it uses all the terminals for random selection?

	unSatTerminals = makeUnSatTerminals(&numUnSatTerminals, terminals, numTerminals, variableValues);

	for(i = 0; i < maxTries; i++){
		// still have tries but dont have any more unchecked variables to check
		if(numUnSatTerminals == 0){
			break;
		}

		// all possible clauses satisfied, i can stop
		if(numTerminals / 2 == *result){
			break;
		}

		lastVariableFlippedIndex = selectRandVariable(unSatTerminals, numUnSatTerminals, reverseLookUpTable);
		flipVariableValue(&variableTable[lastVariableFlippedIndex]);

		updateVariableValues(variableValues, variableTable, numVariableTable);
		currentResult = evaluateAssignments(terminals, numTerminals, variableValues);

		if(currentResult > *result){
			*result = currentResult;
			// rebuild unSatTerminals array
			freePtr(unSatTerminals);
			unSatTerminals = makeUnSatTerminals(&numUnSatTerminals, terminals, numTerminals, variableValues);
		}
		else{
			// flip the value back, it will get updated next time through loop along the next selected variable being tested
			flipVariableValue(&variableTable[lastVariableFlippedIndex]);

			// remove all caluses from unSatTerminals with lastVariableFlippedIndex
			unSatTerminals = removeClausesWithVariable(variableTable[lastVariableFlippedIndex].label, unSatTerminals, &numUnSatTerminals);
			lastVariableFlippedIndex = -1;

			// in case the next increment would exceed maxTries and i had an unsuccessfully last check, i want to restore my previous answer
			if(i == maxTries - 1){
				updateVariableValues(variableValues, variableTable, numVariableTable);
				*result = evaluateAssignments(terminals, numTerminals, variableValues); // extra evaluation but just in case
			}
		}
	}

	freePtr(unSatTerminals);
}

static void localSearchRand(int64_t maxTries, int64_t* result, int64_t* terminals, int64_t numTerminals, struct satVariable* variableTable, int64_t numVariableTable, char* variableValues, int64_t* reverseLookUpTable){
	int64_t i;
	int64_t lastVariableFlippedIndex = -1;
	int64_t currentResult = 0;

	for(i = 0; i < maxTries; i++){
		// all possible clauses satisfied, i can stop
		if(numTerminals / 2 == *result){
			break;
		}

		lastVariableFlippedIndex = selectRandVariable(terminals, numTerminals, reverseLookUpTable);
		flipVariableValue(&variableTable[lastVariableFlippedIndex]);

		updateVariableValues(variableValues, variableTable, numVariableTable);
		currentResult = evaluateAssignments(terminals, numTerminals, variableValues);

		if(currentResult > *result){
			*result = currentResult;
		}
		else{
			// flip the value back, it will get updated next time through loop along the next selected variable being tested
			flipVariableValue(&variableTable[lastVariableFlippedIndex]);

			// in case the next increment would exceed maxTries and i had an unsuccessfully last check, i want to restore my previous answer
			if(i == maxTries - 1){
				updateVariableValues(variableValues, variableTable, numVariableTable);
				*result = evaluateAssignments(terminals, numTerminals, variableValues); // extra evaluation but just in case
			}
		}
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
	int64_t result = 0;
	int64_t maxTries;

	if(argc <= 1){
		perror("ERROR: Must pass in file name of values!");
		perror("Can specify the maximum number of tries for local search with a second parameter, must be integer");
		exit(EXIT_FAILURE);
	}

	terminals = readClauses(argv[1], &numTerminals, &numVariableValues);
	variableValues = findUsedVariables(terminals, numTerminals, numVariableValues, &numVariableTable);
	variableTable = makeVariableTable(terminals, numTerminals, numVariableTable);
	reverseLookUpTable = makeReverseLookUpTable(reverseLookUpTable, numVariableValues, variableTable, numVariableTable);

	// use waited greedy to make a seeder assignment for local search
	greedyAnalysis(terminals, numTerminals, variableTable, numVariableTable, reverseLookUpTable);

	// testPrint(terminals, numTerminals, variableValues, numVariableValues, variableTable, numVariableTable);

	updateVariableValues(variableValues, variableTable, numVariableTable);
	result = evaluateAssignments(terminals, numTerminals, variableValues);
	printf("Greedy Solution: %lld\n", (long long int)result);
	//printValues(variableValues, numVariableValues);

	// use local search as local search algorithm to improve answer, default for maxTries is 1/2 of the number of clauses, i think this is a good default
	// but the default can be over written by the user if so chosen.
	// this version of local search is greedy, we do not choose a random clause, but rather we choose a random clause from all unsatisfied clauses
	maxTries = numTerminals / 2;
	if(argc == 3){
		maxTries = atoi(argv[2]);
	}
	printf("Using %lld as max tries for local search\n", (long long int)maxTries
		);

	localSearch(maxTries, &result, terminals, numTerminals, variableTable, numVariableTable, variableValues, reverseLookUpTable);
	printf("local search Unsat Only Solution: %lld\n", (long long int)result);

	localSearchRand(maxTries, &result, terminals, numTerminals, variableTable, numVariableTable, variableValues, reverseLookUpTable);
	printf("local search random Solution: %lld\n", (long long int)result);

	printValues(variableValues, numVariableValues);

	freePtr(terminals);
	freePtr(variableValues);
	freePtr(reverseLookUpTable);
	for(i = 0; i < numVariableTable; i++){
		freePtr(variableTable[i].indexes);
		freePtr(variableTable[i].negIndexes);
	}
	freePtr(variableTable);

	return 0;
}
