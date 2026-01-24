#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "log_message.h"
#include "ansi_codes.h"

void Message(char *text, char *color) {
	char *line_esc = strchr(text, '\n');
	if(line_esc) *line_esc = '\0';

	// Set text color
	printf("%s", color);

	// Print text
	printf("%s", text);

	// Reset text color
	printf("%s\n", ANSI_WHITE);
}

void MessageError(char *text, char *param) {
	// Set text color to red
	printf("%s", ANSI_RED);

	// Print error message 
	printf("ERROR: %s", text);	

	// Print error parameter (if provided)
	if(param) 
		printf(" [%s]", param);

	// Reset text color, print new line
	printf("%s\n", ANSI_WHITE);
}

void MessageKeyValPair(char *key, char *val) {
	char *line_esc = strchr(val, '\n');
	if(line_esc) *line_esc = '\0';

	// Set text color for key	
	printf("%s", ANSI_BLUE);
	
	// Print key string
	printf("%s", key);

	printf("%s=", ANSI_WHITE);

	// Set text color for value	
	printf("%s", ANSI_YELLOW);
	
	// Print value string
	printf("%s", val);
	
	// Reset text color, print new line
	printf("%s\n", ANSI_WHITE);
}

