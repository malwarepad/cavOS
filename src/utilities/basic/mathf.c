#include "../../../include/string.h"
#include "../../../include/screen.h"
#include "../../../include/util.h"
#include "../../../include/kb.h"
#include "../../../include/math.h"

uint8 mathf_interactive_shell(uint8 id) {
	string prompt = "> ";
	uint8 isReading = 1;

	uint8 chosen = 1;
	uint8 res = -1;

	while (isReading) {
		string tmp = readStr();

		if (cmdEql(tmp, "add")) {
			//uint8 tmp_first = str_to_int(whatIsArgMain(tmp));
			uint8 tmp_first;
			string tmp_tmp = str_to_int(whatIsArgMain(tmp));
			//printf("%s", whatIsArgMain(tmp));
			res = chosen + tmp_first;
		} else if (cmdEql(tmp, "remove")) {
			uint8 tmp_first = str_to_int(whatIsArgMain(tmp));
			res = chosen - tmp_first;
		} else if (cmdEql(tmp, "choose")) {
			uint8 tmp_first = str_to_int(whatIsArgMain(tmp));
			chosen = tmp_first;
		}; printf("\n%d\n", res);
	}
}