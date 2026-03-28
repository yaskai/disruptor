#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum command_path_ids {
	COMM_PATH_QBSP,
	COMM_PATH_VIS,
	COMM_PATH_LIGHT
};

char *command_paths[3] = { NULL };

int main(int argc, char **argv) {
	printf("------------------ COMPILE MAP ------------------\n");
	printf("reading config...\n");

	// Parse options file
	FILE *pf = fopen("compile_map.conf", "r");	
		
	if(!pf) {
		printf("ERROR: compile_map.conf was found\n");
		return 1;
	}

	char line[255];
	while(fgets(line, sizeof(line), pf)) {
		char *key = line;	
		if(!key) continue;

		char *sep = strchr(line, '='); 
		if(!sep) continue;
		*sep = '\0';

		char *val = sep + 1;
		if(!val) continue;

		char *tok = strtok(val, "\"");
		if(!tok) continue;

		// Set command paths 
		if(strcmp(key, "qbsp") == 0) {
			// qbsp
			command_paths[COMM_PATH_QBSP] = tok;
			printf("qbsp set to %s\n", command_paths[COMM_PATH_QBSP]);

		} else if(strcmp(key, "vis") == 0) {
			// vis
			command_paths[COMM_PATH_VIS] = tok;
			printf("vis set to %s\n", command_paths[COMM_PATH_VIS]);

		} else if(strcmp(key, "light") == 0) {
			// light
			command_paths[COMM_PATH_LIGHT] = tok;
			printf("light set to %s\n", command_paths[COMM_PATH_LIGHT]);
		}

	}

	fclose(pf);

	return 0;
}
