
// struct Symbol {
//     string id;
//     TokenType type;
//     bool publicVar;
// };
// struct SymbolTable {
//     unordered_map<string, Symbol*> symbols;  // Hashtable for O(1) lookup
//     SymbolTable* parent;  // pointer to parent symbol table (if nested scopes)
// };
// vector<SymbolTable> symbolTableStack;
// vector<string> symbolsToType;
// SymbolTable* currentTable;
// Node* parse_unaryOperator(Node* node);
// Node* parse_binaryOperator(Node* node);
// Token parse_primary(Node* node);

#ifndef __PARSER__H__
#define __PARSER__H__
// #define DEBUG
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "inputbuf.h"
#include "lexer.h"
#include <algorithm>

using namespace std;

struct Node {
    string name;
    TokenType type = TokenType::UNKNOWN;
    vector<Node*> children;
};

struct Constraint {
    string var1;
    string var2;
};

class Parser {
    public:
        LexicalAnalyzer lexer;
        Token token;
        TokenType tempTokenType;
        unordered_map<string,TokenType> typedVariables;
        vector<string> variables;
        vector<pair<string,string>> constraints;
        vector<vector<string>> constraintGroups;
        vector<vector<string>> constraintGroupsSorted;
        vector<string> finalPrint;
        Node* ast_root;
        bool ifParent = false;
        bool whileParent = false;
        bool switchParent = false;
        int basicIncrement = 0;
        int compareIncrement = 0;
        int notIncrement = 0;
        int increment = 0;

        void print_ast(Node* node, string indent) {
            if (node) {
                cout << indent << node->name << "-" << node->type << endl;
                for (auto child : node->children) {
                    print_ast(child, indent + "  ");
                }
            }
        }

        void deallocate_tree(Node* node) {
            if (node) {
                for (auto child : node->children) {
                    deallocate_tree(child);
                }
                delete node;
            }
        }
        void updateTypedVariables(Node* node) {
            if (node){
                if (typedVariables[node->name] && node->type != TokenType::UNKNOWN){
                    typedVariables[node->name] = node->type;
                }
                for(auto child : node->children) {
                    updateTypedVariables(child);
                }
            }
        }

        void combinePairsIntoGroups() {
            unordered_map<string, vector<string>> adjacencyList;
            unordered_set<string> visited;

            for(const auto& constraint : constraints) {
                adjacencyList[constraint.first].push_back(constraint.second);
                adjacencyList[constraint.second].push_back(constraint.first);
            }

            for(const auto& node : adjacencyList) {
                if(visited.count(node.first) == 0) {
                    vector<string> group;
                    queue<string> queue;
                    queue.push(node.first);
                    visited.insert(node.first);

                    while(!queue.empty()) {
                        string current = queue.front(); queue.pop();
                        group.push_back(current);

                        for(const string& neighbour : adjacencyList[current]) {
                            if(visited.count(neighbour) == 0) {
                                queue.push(neighbour);
                                visited.insert(neighbour);
                            }
                        }
                    }
                    constraintGroups.push_back(group);
                }
            }
            removeBasic();
            sortGroups();
            makeFinalPrint();
        }

        void removeBasic() {
            for(auto& group : constraintGroups) {
                vector<string> newGroup;
                for(const auto& s : group) {
                    if(s.find("BASIC") == string::npos) {
                        newGroup.push_back(s);
                    }
                }
                group = newGroup;
            }
        }
        void sortGroups() {
            vector<vector<string>> constraintGroupsReversed;
            for(int i = constraintGroups.size() - 1; i >= 0; i--) {
                constraintGroupsReversed.push_back(constraintGroups[i]);
                constraintGroupsSorted.push_back(vector<string>());
            }
            for (const auto& varOuter : variables) {
                int constraintGroupCounter = 0;
                for (const auto& group : constraintGroupsReversed) {
                    for (const auto& varInner : group) {
                        if (varOuter == varInner) {
                            constraintGroupsSorted[constraintGroupCounter].push_back(varInner);
                        }
                    }
                    constraintGroupCounter++;
                }
            }
            constraintGroups = constraintGroupsSorted;
        }
        void printGroups() {
            for(int i = 0; i < constraintGroups.size(); ++i) {
                cout << "Group " << i + 1 << ": ";
                for(const auto& str : constraintGroups[i]) {
                    cout << str << " ";
                }
                cout << endl;
            }
        }
        void makeFinalPrint() {
            unordered_map<string, string> typedVars;
            unordered_map<string, int> groupNums;
            int groupNum = 0;
            // for(const auto& vars : typedVariables){
            //     cout << vars.first << " ::: " << vars.second << endl;
            // }
            updateTypedVariables(ast_root);
            for(const auto& group : constraintGroups) {
                bool typeFound = false;
                string typeString;
                for(const auto& var : group) {
                    // cout << var << " " << typedVariables[var] << endl;
                    if (typedVariables[var] != TokenType::UNKNOWN) {
                        typeFound = true;
                        if (typedVariables[var] == TokenType::INT) {
                            typeString = "int";
                        } else if (typedVariables[var] == TokenType::REAL) {
                            typeString = "real";
                        } else if (typedVariables[var] == TokenType::BOO) {
                            typeString = "bool";
                        }
                        break;
                    }
                }

                if(typeFound) {
                    for(const auto& var : group) {
                        typedVars[var] = typeString;
                        groupNums[var] = groupNum;
                    }
                }

                if(!typeFound) {
                    for(const auto& var : group) {
                        groupNums[var] = groupNum;
                    }
                }
                groupNum++;
            }

            // Create final print according to variable order
            unordered_set<int> printedGroups;
            for(const auto& var : variables) {
                if(typedVars.count(var) > 0) {
                    finalPrint.push_back(var + ": " + typedVars[var] + " #");
                } else if(groupNums.count(var) > 0) {
                    int gNum = groupNums[var];
                    if(printedGroups.count(gNum) == 0) {
                        string groupString = "";
                        for(const auto& v : constraintGroups[gNum]) {
                            groupString += v;
                            if(v != constraintGroups[gNum].back()) {
                                groupString += ", ";
                            }
                        }
                        groupString += ": ? #";
                        finalPrint.push_back(groupString);
                        printedGroups.insert(gNum);
                    }
                } else {
                    string typeString;
                    TokenType type = typedVariables[var];
                    if (type == TokenType::INT) {
                        typeString = "int";
                    } else if (type == TokenType::REAL) {
                        typeString = "real";
                    } else if (type == TokenType::BOO) {
                        typeString = "bool";
                    }
                    finalPrint.push_back(var + ": " + typeString + " #");
                }
            }
        }



        Node* parse_program();
    private:
        Node* parse_varlist(Node* node);
        Node* parse_expression(Node* node);
        Node* parse_assstmt(Node* node);
        Node* parse_case(Node* node);
        Node* parse_caselist(Node* node);
        Node* parse_switchstmt(Node* node);
        Node* parse_whilestmt(Node* node);
        Node* parse_ifstmt(Node* node);
        Node* parse_stmt(Node* node);
        Node* parse_stmtlist(Node* node);
        Node* parse_body(Node* node);
        Node* parse_typename(Node* node);
        Node* parse_vardecl(Node* node);
        Node* parse_vardecllist(Node* node);
        Node* parse_globalVars(Node* node);
};

#endif  //__PARSER__H__