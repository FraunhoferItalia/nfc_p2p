#include "BooleanParser.h"

BooleanParser::BooleanParser(void)
{

}

uint8_t BooleanParser::get_precedence(char c)
{
	return (c == '&') ? 1 : (c == '|') ? 2 : (c == '!') ? 3 : 4;
}


char BooleanParser::check_type(char c)
{
	return (c == 33 || c == 38 || c == 124) ? 'o' : isalpha(c) ? 'c' : (isdigit(c) || c == '.') ? 'd' : (c == 40 || c == 41) ? 'p' : (c == 44) ? 's' : 0;
}

uint8_t BooleanParser::ParseIt(char* toBeConverted,uint8_t executed[], uint8_t lenExecuted)
{
	//printf("Type your expression(no spaces):\n");
	memcpy(expression, toBeConverted, sizeof(expression));
	get_rpn();
	//printf("Schunting Yard notation: %s\n", output);
	
	eval_rpn(executed, lenExecuted);
	//printf("answer: %d \n", answer);
	
	memset(st1.tok_stack, 0, sizeof(st1.tok_stack));
	memset(output, 0, sizeof(output));
	out_length = 0;
	return answer;

}

void BooleanParser::get_rpn()
{
	st1.size = 0;
	uint8_t z;
	size_t x;
	token o1;
	token o2;
	for (x = 0; x<strlen(expression); x++)
	{
		//////////////////////////////////////////////////////////
		if (check_type(expression[x]) == 'd')
		{
			output[out_length++] = expression[x];
			continue;
		}
		/////////////////////////////////////////////////////////
		if (check_type(expression[x]) == 'o')
		{
			if (output[out_length - 1] != ' ')
				output[out_length++] = ' ';
			o1.token_str[0] = expression[x];
			o1.assoc = get_assoc(expression[x]);
			o1.precedence = get_precedence(expression[x]);
			o2 = st1.tok_stack[st1.size];
			while (check_type(o2.token_str[0]) == 'o')
			{
				o2.precedence = get_precedence(o2.token_str[0]);
				if ((o1.assoc == left && o1.precedence == o2.precedence) || (o1.precedence<o2.precedence))
				{
					output[out_length++] = pop().token_str[0];
					if (output[out_length - 1] != ' ')
						output[out_length++] = ' ';
				}
				else
					break;
				o2 = st1.tok_stack[st1.size];
			}
			push(o1);
			continue;
		}
		////////////////////////////////////////////////////////////
		if (expression[x] == 40)
		{
			o1.token_str[0] = expression[x];
			o1.assoc = get_assoc(expression[x]);
			o1.precedence = get_precedence(expression[x]);
			o1.token_type = 'p';
			push(o1);
			continue;
		}
		////////////////////////////////////////////////////////////
		if (expression[x] == 41)
		{
			while ((st1.tok_stack[st1.size]).token_str[0] != 40 && st1.size>0)
			{
				if (output[out_length - 1] != ' ')
					output[out_length++] = ' ';
				output[out_length++] = pop().token_str[0];
				if (output[out_length - 1] != ' ')
					output[out_length++] = ' ';
			}
			if (st1.size == -1)
			{
				printf("Paranthesis mismatch!");
				break;
			}
			pop();
			if ((st1.tok_stack[st1.size]).token_type == 'c')
			{
				if (output[out_length - 1] != ' ')
					output[out_length++] = ' ';
				strcat(output, pop().token_str);
				out_length = strlen(output);
			}
		}


		///////////////////////////////////////////////////////////
		if (check_type(expression[x]) == 'c')
		{
			z = 0;
			while (check_type(expression[x]) == 'c')
			{
				o1.token_str[z++] = expression[x++];
			}
			x--;
			o1.token_str[z] = '\0';
			o1.token_type = 'c';
			push(o1);

		}
		///////////////////////////////////////////////////////////
		if (check_type(expression[x]) == 's')
		{

			if (output[out_length - 1] != ' ')
				output[out_length++] = ' ';
			continue;
		}
		//////////////////////////////////////////////////////////
	}

	while (st1.size>0)
	{
		if (output[out_length - 1] != ' ')
			output[out_length++] = ' ';
		if (check_type(expression[x]) == 'p')
			printf("Paranthesis mismatch!");
		output[out_length++] = pop().token_str[0];
	}
	output[out_length++] = pop().token_str[0];


	strcpy(expression, output);

}


void BooleanParser::do_op(token temp,uint8_t executed[],uint8_t lenExecuted)
{
	uint8_t p1 = 0;
	uint8_t p2 = 0;
	uint8_t ans = 0;
	char type = check_type(temp.token_str[0]);
	if (type == 'd')
		push(temp);
	else
	{
		token tempo = { NULL,NULL,0,0 };

		if (strcmp(temp.token_str, "!") == 0)
		{
			p1 = atoi(pop().token_str);
			 ans = !(p1);
			sprintf(tempo.token_str, "%d", ans);
		}
		else if (strcmp(temp.token_str, "&") == 0)
		{
			 p1 = atoi(pop().token_str);
			 p2 = atoi(pop().token_str);
			 ans = p2 && p1;
			sprintf(tempo.token_str, "%d", ans);
		}
		else if (strcmp(temp.token_str, "|") == 0)
		{
			 p1 = atoi(pop().token_str);
			 p2 = atoi(pop().token_str);
			 ans = p2 || p1;
			sprintf(tempo.token_str, "%d", ans);
		}
		else if (strcmp(temp.token_str, "im") == 0) // equivalent of "is member"
		{
			 ans;
			 p1 = atoi(pop().token_str);
			for (uint8_t i = 0; i < lenExecuted; i++)
			{
				if (executed[i] == p1) {
					ans = 1;
					break;
				}
				else
					ans = 0;
			}
			sprintf(tempo.token_str, "%d", ans);
		}
		push(tempo);
	}
}



void BooleanParser::eval_rpn(uint8_t executed[], uint8_t lenExecuted)
{
	st1.size = 0;
	token temp;
	uint8_t z = 0;
	size_t x;
	for (x = 0; x<strlen(expression); x++)
	{
		if (expression[x] == ' ')
		{
			temp.token_str[z] = '\0';
			do_op(temp,executed,lenExecuted);
			z = 0;
		}
		else
		{
			temp.token_str[z++] = expression[x];
			if (x == strlen(expression) - 1)
			{
				temp.token_str[z] = '\0';
				do_op(temp,executed, lenExecuted);
			}
		}
	}
	answer = atoi(pop().token_str);
}

