#include "../../../include/string.h"
#include "../../../include/screen.h"
#include "../../../include/util.h"
#include "../../../include/kb.h"
#include "../../../include/math.h"

void mathf_interactive_shell(uint8 id) {
	string prompt = "> ";
	uint8 isReading = 1;

	uint8 chosen = 1;
	uint8 res = -1;

	printf("\n");
	while (isReading == 1) {
		printf("%s", prompt);
		string tmp = readStr();

		if (cmdEql(tmp, "add")) {
			uint8 tmp_first = str_to_int(whatIsArgMain(tmp));
			//printf("%s", whatIsArgMain(tmp));
			res = chosen + tmp_first;
			printf("\n%d\n", res);
		} else if (cmdEql(tmp, "remove")) {
			uint8 tmp_first = str_to_int(whatIsArgMain(tmp));
			if (chosen >= tmp_first) {
				res = chosen - tmp_first;
				printf("\n%d\n", res);
			} else if (chosen < tmp_first) {
				res = tmp_first - chosen;
				printf("\n-%d\n", res);
			}
		} else if (cmdEql(tmp, "multiply")) {
			uint8 tmp_first = str_to_int(whatIsArgMain(tmp));
			res = chosen * tmp_first;
			printf("\n%d\n", res);
		} else if (cmdEql(tmp, "dia")) {
			uint8 tmp_first = str_to_int(whatIsArgMain(tmp));
			res = chosen / tmp_first;
			printf("\n%d\n", res);
		} else if (cmdEql(tmp, "choose")) {
			uint8 tmp_first = str_to_int(whatIsArgMain(tmp));
			chosen = tmp_first;
			printf("\n");
		} else if (cmdEql(tmp, "ret")) {
			isReading = 0;
			printf("\n");
		} else if (cmdEql(tmp, "help")) {
			printf("\nBasic Commands:");
			printf("\nchoose    : Choose a number as a base");
			printf("\nret       : Return back to the regular shell");

			printf("\n\nMathematical equations:");
			printf("\nadd       : +");
			printf("\nremove    : -");
			printf("\nmultiply  : *");
			printf("\ndia       : /");

			printf("\n");
		} else {
			printf("\n(MATHF) %s isn't a valid command\n", tmp);
		}
	}
}
