/*
 * Template Created by: Joshua Elkins
 * Modified by: Ari Argoud 
 * Date: June 24th, 2023
 */

#include <iostream>
#include <string>
#include "parser.h"
#include "lexer.h"
#include "inputbuf.h"
using namespace std;
string typeErrorString = "type error";
void syntax_error(){
	cout << "Syntax Error\n";
	exit(1);
}
void type_error(){
	cout << typeErrorString << "\n";
	exit(1);
}

Node* Parser::parse_program(){
	#ifdef DEBUG
		cout << "Entered Parse Program" << endl;
	#endif
	ast_root = new Node();
	ast_root->name = "PROGRAM";
	token = lexer.GetToken();
	if(token.token_type == ID){
		lexer.UngetToken(token);
		Node* globalVarsNode = parse_globalVars(ast_root);
		Node* bodyNode = parse_body(ast_root);
		ast_root->children.push_back(globalVarsNode);
		ast_root->children.push_back(bodyNode);
    }
	else if(token.token_type == LBRACE){
		lexer.UngetToken(token);
		Node* bodyNode = parse_body(ast_root);
		ast_root->children.push_back(bodyNode);
	}
	else{
		syntax_error();
	}
	return ast_root;
}

Node* Parser::parse_globalVars(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Global Variables" << endl;
	#endif
	Node* globalVarsNode = new Node();
    globalVarsNode->name = "GlobalVars";
	parse_vardecllist(globalVarsNode);
	return globalVarsNode;
}

Node* Parser::parse_vardecllist(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Variable Declaration List" << endl;
	#endif
	token = lexer.GetToken();
	while(token.token_type == ID){
		lexer.UngetToken(token);
		parse_vardecl(parent);
		token = lexer.GetToken();
	}
	lexer.UngetToken(token);
	return parent;
}

Node* Parser::parse_vardecl(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Variable Declaration" << endl;
	#endif
	Node* varDeclNode = new Node();
	varDeclNode->name = "VarDecl";
	token = lexer.GetToken();
	if(token.token_type != ID){
		syntax_error();
	}
	lexer.UngetToken(token);
	parse_varlist(varDeclNode);
	token = lexer.GetToken();
	if(token.token_type != COLON){
		syntax_error();
	}
	token = lexer.GetToken();
	if(token.token_type == INT || token.token_type == REAL || token.token_type == BOO){
		lexer.UngetToken(token);
		parse_typename(varDeclNode);
		token = lexer.GetToken();
		if(token.token_type != SEMICOLON){
			syntax_error();
		}
	}
	else{
		syntax_error();
	}
	parent->children.push_back(varDeclNode);

	int size = varDeclNode->children.size()-1;
	TokenType type;
	if (varDeclNode->children[size]->name == "int"){
		type = INT;
	}
	else if (varDeclNode->children[size]->name == "real"){
		type = REAL;
	}
	else if (varDeclNode->children[size]->name == "bool"){
		type = BOO;
	}
	else{
		type = UNKNOWN;
	}
	int i = 0;
	while(i<size){
		typedVariables[varDeclNode->children[i]->name] = type;
		i++;
	}
	return varDeclNode;
}

Node* Parser::parse_varlist(Node* parent) {
    #ifdef DEBUG
        cout << "Entered Parse Variable List" << endl;
    #endif
    token = lexer.GetToken();
    if(token.token_type != ID){
        syntax_error();
    }
    else {
        Token t2 = lexer.GetToken();
		if(t2.token_type == COLON){
			Node* varIDNode = new Node();
			varIDNode->name = token.lexeme;
			parent->children.push_back(varIDNode);
			variables.push_back(token.lexeme);
			lexer.UngetToken(t2);
			return parent;
		}
        else if(t2.token_type == COMMA){
            do {
				Node* varIDNode = new Node();
				varIDNode->name = token.lexeme;
				parent->children.push_back(varIDNode);
				variables.push_back(token.lexeme);
                token = lexer.GetToken();
                if(token.token_type != ID) {
					cout << "problem 2" << endl;
                    syntax_error();
                }
                t2 = lexer.GetToken();
            } while(token.token_type == ID && t2.token_type == COMMA);
            lexer.UngetToken(t2);
        }
		Node* varIDNode = new Node();
		varIDNode->name = token.lexeme;
		variables.push_back(token.lexeme);
		parent->children.push_back(varIDNode);
    }
    return parent;
}

Node* Parser::parse_typename(Node* parent){

    #ifdef DEBUG
        cout << "Entered Parse Type Name" << endl;
    #endif
    token = lexer.GetToken();
    if(token.token_type == INT || token.token_type == REAL || token.token_type == BOO){
		Node* varTypeNode = new Node();
		varTypeNode->name = token.lexeme;
		parent->children.push_back(varTypeNode);
    }
    else{
        syntax_error();
    }
    return parent;
}

Node* Parser::parse_body(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Body" << endl;
	#endif
	Node* bodyNode = new Node();
	bodyNode->name = "Body";
	token = lexer.GetToken();
	if(token.token_type == LBRACE){
		parse_stmtlist(bodyNode);
		token = lexer.GetToken();
		if(token.token_type != RBRACE){
			syntax_error();
		}
	}
	else{
		syntax_error();
	}
	return bodyNode;
}

Node* Parser::parse_stmtlist(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Statement List" << endl;
	#endif
	token = lexer.GetToken();
	while(token.token_type == ID || token.token_type == IF || token.token_type == WHILE || token.token_type == SWITCH){
		lexer.UngetToken(token);
		parse_stmt(parent);
		token = lexer.GetToken();
	}
	lexer.UngetToken(token);
	return parent;
}

Node* Parser::parse_stmt(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Statement" << endl;
	#endif
	token = lexer.GetToken();
	if(token.token_type == ID){
		lexer.UngetToken(token);
		Node* assignmentNode = parse_assstmt(parent);
		parent->children.push_back(assignmentNode);
	}
	else if(token.token_type == IF){
		lexer.UngetToken(token);
		Node* ifNode = parse_ifstmt(parent);
		ifNode->name = "If";
		parent->children.push_back(ifNode);
	}
	else if(token.token_type == WHILE){
		lexer.UngetToken(token);
		Node* whileNode = parse_whilestmt(parent);
		whileNode->name = "While";
		parent->children.push_back(whileNode);
	}
	else if(token.token_type == SWITCH){
		lexer.UngetToken(token);
		Node* switchNode = parse_switchstmt(parent);
		switchNode->name = "Switch";
		parent->children.push_back(switchNode);	
	}
	else{
		syntax_error();
	}
	return parent;
}

Node* Parser::parse_assstmt(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Assignment Statement" << endl;
	#endif
	Node* assignmentNode = new Node();
	assignmentNode->name = "Assignment";
	token = lexer.GetToken();
	if(token.token_type != ID){
		syntax_error();
	}
	Node* assignmentIDNode = new Node();
	assignmentIDNode->name = token.lexeme;
	if (typedVariables[token.lexeme]){
		assignmentIDNode->type = typedVariables[token.lexeme];
	}
	else{
		assignmentIDNode->type = UNKNOWN;
		variables.push_back(token.lexeme);
		typedVariables[token.lexeme] = UNKNOWN;
	}

	assignmentNode->children.push_back(assignmentIDNode);
	token = lexer.GetToken();
	if(token.token_type != EQUAL){
		syntax_error();
	}
	Node* expressionNode = parse_expression(assignmentNode);
	assignmentNode->children.push_back(expressionNode);
	token = lexer.GetToken();
	if(token.token_type != SEMICOLON){
		syntax_error();
	}
	
	if (assignmentNode->children[0]->type == assignmentNode->children[1]->type){
	}
	else if ((assignmentNode->children[0]->type == UNKNOWN || assignmentNode->children[1]->type == UNKNOWN) 
			&&(assignmentNode->children[0]->type != UNKNOWN || assignmentNode->children[1]->type != UNKNOWN)){

		if (assignmentNode->children[0]->type != UNKNOWN){
			typedVariables[assignmentNode->children[1]->name] = assignmentNode->children[0]->type;
			assignmentNode->children[1]->type = assignmentNode->children[0]->type;
		}
		else if (assignmentNode->children[1]->type != UNKNOWN){
			typedVariables[assignmentNode->children[0]->name] = assignmentNode->children[1]->type;
			assignmentNode->children[0]->type = assignmentNode->children[1]->type;
		}

	}
	else{
		typeErrorString = "TYPE MISMATCH " + to_string(token.GetLineNo()) + " C1";
		type_error();
	}
	if (assignmentNode->children[0]->type == UNKNOWN && assignmentNode->children[1]->type == UNKNOWN) {
		constraints.push_back(make_pair(assignmentNode->children[0]->name, assignmentNode->children[1]->name));
		if (assignmentNode->children[1]->name.find("BASIC") != std::string::npos 
			|| assignmentNode->children[1]->name.find("COMPARE") != std::string::npos 
			|| assignmentNode->children[1]->name.find("NOT") != std::string::npos) {
			string pair;
			if (assignmentNode->children[1]->children[1]->name.find("BASIC") == std::string::npos 
				&& assignmentNode->children[1]->children[1]->name.find("COMPARE") == std::string::npos 
				&& assignmentNode->children[1]->children[1]->name.find("NOT") == std::string::npos) {
				pair = assignmentNode->children[1]->children[1]->name;
			}
			else{
				pair = assignmentNode->children[1]->children[0]->name;
			}
			constraints.push_back(make_pair(assignmentNode->children[1]->name, pair));
		}

	}

	return assignmentNode;
}

Node* Parser::parse_expression(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Expression" << endl;
	#endif
	Node* expressionNode = new Node();
	token = lexer.GetToken();
	if(token.token_type == NOT){
		expressionNode->name = "NOT"+to_string(notIncrement);

		Node* expressionNode1 = parse_expression(expressionNode);
		expressionNode->children.push_back(expressionNode1);
		if (expressionNode->children[0]->type == BOO){
			expressionNode->type = BOO;
		}
		else if (expressionNode->children[0]->type == UNKNOWN){
			expressionNode->type = BOO;
			expressionNode->children[0]->type = BOO;
			typedVariables[expressionNode->children[0]->name] = BOO;
		}
		else {
			typeErrorString = "TYPE MISMATCH " + to_string(token.GetLineNo()) + " C3";
			type_error();
		}
	}
	else if(token.token_type == PLUS || token.token_type == MINUS || token.token_type == MULT ||  token.token_type == DIV){
		expressionNode->name = "BASIC" + to_string(basicIncrement);
		basicIncrement++;
		Node* expressionNode1 = parse_expression(expressionNode);
		expressionNode->children.push_back(expressionNode1);
		Node* expressionNode2 = parse_expression(expressionNode);
		expressionNode->children.push_back(expressionNode2);

		if (expressionNode->children[0]->type == expressionNode->children[1]->type){
			expressionNode->type = expressionNode->children[0]->type; 
		}
		else if ((expressionNode->children[0]->type == UNKNOWN || expressionNode->children[1]->type == UNKNOWN)
				&& (expressionNode->children[0]->type != UNKNOWN || expressionNode->children[1]->type != UNKNOWN)){
			if(expressionNode->children[0]->type != UNKNOWN){
				typedVariables[expressionNode->children[1]->name] = expressionNode->children[0]->type;
				expressionNode->children[1]->type = expressionNode->children[0]->type;
				expressionNode->type = expressionNode->children[0]->type;
			}
			else if (expressionNode->children[1]->type != UNKNOWN){
				typedVariables[expressionNode->children[0]->name] = expressionNode->children[1]->type;
				expressionNode->children[0]->type = expressionNode->children[1]->type;
				expressionNode->type = expressionNode->children[1]->type;
			}
		}
		else{
			typeErrorString = "TYPE MISMATCH " + to_string(token.GetLineNo()) + " C2";
			type_error();
		}
		if (expressionNode->children[0]->type == UNKNOWN && expressionNode->children[1]->type == UNKNOWN){
			constraints.push_back(make_pair(expressionNode->children[0]->name, expressionNode->children[1]->name));
			if (expressionNode->children[1]->name.find("BASIC") != std::string::npos) {
				string pair;
				if (expressionNode->children[1]->children[1]->name.find("BASIC") == std::string::npos) {
					pair = expressionNode->children[1]->children[1]->name;
				}
				else{
					pair = expressionNode->children[1]->children[0]->name;
				}
				constraints.push_back(make_pair(expressionNode->children[1]->name, pair));
			}
			else if (expressionNode->children[0]->name.find("BASIC") != std::string::npos){
				string pair;
				if (expressionNode->children[0]->children[1]->name.find("BASIC") == std::string::npos) {
					pair = expressionNode->children[0]->children[1]->name;
				}
				else{
					pair = expressionNode->children[0]->children[0]->name;
				}
				constraints.push_back(make_pair(expressionNode->children[0]->name, pair));
			}
		}
	}
	else if(token.token_type == GREATER || token.token_type == LESS || token.token_type == GTEQ || token.token_type == LTEQ || token.token_type == EQUAL || token.token_type == NOTEQUAL){
		expressionNode->name = "COMPARE" + to_string(compareIncrement);
		compareIncrement++;
		Node* expressionNode1 = parse_expression(expressionNode);
		expressionNode->children.push_back(expressionNode1);
		Node* expressionNode2 = parse_expression(expressionNode);
		expressionNode->children.push_back(expressionNode2);

		if (expressionNode->children[0]->type == expressionNode->children[1]->type){
			expressionNode->type = BOO; 
		}
		else if ((expressionNode->children[0]->type == UNKNOWN || expressionNode->children[1]->type == UNKNOWN)
				&& (expressionNode->children[0]->type != UNKNOWN || expressionNode->children[1]->type != UNKNOWN)){
			if(expressionNode->children[0]->type != UNKNOWN){
				typedVariables[expressionNode->children[1]->name] = expressionNode->children[0]->type;
				expressionNode->children[1]->type = expressionNode->children[0]->type;
				expressionNode->type = expressionNode->children[0]->type;
			}
			else if (expressionNode->children[1]->type != UNKNOWN){
				typedVariables[expressionNode->children[0]->name] = expressionNode->children[1]->type;
				expressionNode->children[0]->type = expressionNode->children[1]->type;
				expressionNode->type = expressionNode->children[1]->type;
			}
		}
		else{
			type_error();
		}	
		if (expressionNode->children[0]->type == UNKNOWN && expressionNode->children[1]->type == UNKNOWN){
			constraints.push_back(make_pair(expressionNode->children[0]->name, expressionNode->children[1]->name));
			if (expressionNode->children[1]->name.find("BASIC") != std::string::npos) {
				string pair;
				if (expressionNode->children[1]->children[1]->name.find("BASIC") == std::string::npos) {
					pair = expressionNode->children[1]->children[1]->name;
				}
				else{
					pair = expressionNode->children[1]->children[0]->name;
				}
				constraints.push_back(make_pair(expressionNode->children[1]->name, pair));
			}
			else if (expressionNode->children[0]->name.find("BASIC") != std::string::npos) {
				string pair;
				if (expressionNode->children[0]->children[1]->name.find("BASIC") == std::string::npos) {
					pair = expressionNode->children[0]->children[1]->name;
				}
				else{
					pair = expressionNode->children[0]->children[0]->name;
				}
				constraints.push_back(make_pair(expressionNode->children[0]->name, pair));
			}
		}
	}
	else if(token.token_type == ID || token.token_type == NUM || token.token_type == REALNUM || token.token_type == TR || token.token_type == FA){
		expressionNode->name = token.lexeme;
		if(typedVariables[token.lexeme]){
			expressionNode->type = typedVariables[token.lexeme];
		}
		else if (token.token_type == ID){
			expressionNode->type = UNKNOWN;
			typedVariables[token.lexeme] = UNKNOWN;
			variables.push_back(token.lexeme);
		}
		else if(token.token_type == REALNUM){
			expressionNode->type = REAL;
		}
		else if (token.token_type == NUM){
			expressionNode->type = INT;
		}
		else if (token.token_type == TR || FA){
			expressionNode->type = BOO;
		}



	}
	else{
		syntax_error();
	}
	return expressionNode;
}

Node* Parser::parse_ifstmt(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse If Statement" << endl;
	#endif
	Node* ifNode = new Node();
	ifNode->name = "If";
	ifParent = true;
	token = lexer.GetToken();
	if(token.token_type != IF){
		syntax_error();
	}
	token = lexer.GetToken();
	if(token.token_type != LPAREN){
		syntax_error();
	}
	Node* expressionNode = parse_expression(ifNode);
	ifNode->children.push_back(expressionNode);
	token = lexer.GetToken();

	if(ifNode->children[0]->type == BOO){
	}
	else if (ifNode->children[0]->type == UNKNOWN){
		ifNode->children[0]->type = BOO;
	}
	else{
		typeErrorString = "TYPE MISMATCH " + to_string(token.GetLineNo()) + " C4";
		type_error();
	}
	if(token.token_type != RPAREN){
		syntax_error();
	}

	Node* bodyNode = parse_body(ifNode);
	ifNode->children.push_back(bodyNode);
	ifParent = false;

	return ifNode;
}

Node* Parser::parse_whilestmt(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse While Statement" << endl;
	#endif
	Node* whileNode = new Node();
	whileNode->name = "While";
	whileParent = true;
	token = lexer.GetToken();
	if(token.token_type != WHILE){
		syntax_error();
	}
	token = lexer.GetToken();
	if(token.token_type != LPAREN){
		syntax_error();
	}
	Node* expressionNode = parse_expression(whileNode);
	whileNode->children.push_back(expressionNode);

	if(whileNode->children[0]->type == BOO){
	}
	else if(whileNode->children[0]->type == UNKNOWN){
		whileNode->children[0]->type = BOO;
	}
	else{
		typeErrorString = "TYPE MISMATCH " + to_string(token.GetLineNo()) + " C4";
		type_error();
	}

	token = lexer.GetToken();
	if(token.token_type != RPAREN){
		syntax_error();
	}
	Node* bodyNode = parse_body(whileNode);
	whileNode->children.push_back(bodyNode);
	whileParent = false;

	return whileNode;
}

Node* Parser::parse_switchstmt(Node* parent){
	#ifdef DEBUG
		cout << "Entered Switch Statement" << endl;
	#endif
	Node* switchNode = new Node();
	switchNode->name = "Switch";
	switchParent = true;
	token = lexer.GetToken();
	if(token.token_type != SWITCH){
		syntax_error();
	}
	token = lexer.GetToken();
	if(token.token_type != LPAREN){
		syntax_error();
	}
	Node* expressionNode = parse_expression(switchNode);
	switchNode->children.push_back(expressionNode);

	if(switchNode->children[0]->type == INT){
	}
	else if(switchNode->children[0]->type == UNKNOWN){
		switchNode->children[0]->type = INT;
	}
	else{
		typeErrorString = "TYPE MISMATCH " + to_string(token.GetLineNo()) + " C5";
		type_error();
	}

	token = lexer.GetToken();
	if(token.token_type != RPAREN){
		syntax_error();
	}
	token = lexer.GetToken();
	if(token.token_type != LBRACE){
		syntax_error();
	}
	parse_caselist(switchNode);
	token = lexer.GetToken();
	if(token.token_type != RBRACE){
		syntax_error();
	}
	switchParent = false;

	return switchNode;
}

Node* Parser::parse_caselist(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Case List" << endl;
	#endif
	token = lexer.GetToken();
	if(token.token_type == CASE){
		while(token.token_type == CASE){
			lexer.UngetToken(token);
			parse_case(parent);
			token = lexer.GetToken();
		}
		lexer.UngetToken(token);
	}
	else{
		syntax_error();
	}
	return parent;
}

Node* Parser::parse_case(Node* parent){
	#ifdef DEBUG
		cout << "Entered Parse Case" << endl;
	#endif
	token = lexer.GetToken();
	if(token.token_type != CASE){
		syntax_error();
	}
	token = lexer.GetToken();
	if(token.token_type != NUM){
		syntax_error();
	}
	token = lexer.GetToken();
	if(token.token_type != COLON){
		syntax_error();
	}
	Node* bodyNode = parse_body(parent);
	parent->children.push_back(bodyNode);
	return parent;
}

int main(){
	#ifdef DEBUG
		cout << "Entered Main" << endl;
	#endif
	Node* i;
    	Parser* parseProgram = new Parser();
    	i = parseProgram->parse_program();

		parseProgram->combinePairsIntoGroups();
		

		for (const auto& print : parseProgram->finalPrint){
			std::cout << print << std::endl;
		}
		parseProgram->deallocate_tree(parseProgram->ast_root);
	return 0;	
}
