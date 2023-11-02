#include <iostream>
#include <fstream>
#include <stack>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype> // 为了使用 isalpha()
#include <queue>
#include <set>
#include <map>

struct Transition
{
    char symbol;
    class State *target;

    Transition(char _symbol, class State *_target) : symbol(_symbol), target(_target) {}
};

class State
{
public:
    int id;
    bool isFinal;
    std::vector<Transition> transitions;

    State(int _id, bool _isFinal = false) : id(_id), isFinal(_isFinal) {}
};

class NFA
{
public:
    class State *start;
    class State *accept;

    NFA(class State *_start, class State *_accept) : start(_start), accept(_accept) {}
};

int stateCount = 0;

class State *createState(bool isFinal = false)
{
    return new class State(stateCount++, isFinal);
}

void addTransition(class State *from, class State *to, char symbol)
{
    from->transitions.push_back(Transition(symbol, to));
}

class NFA *thompsonConstruction(char inputChar)
{
    class State *startState = createState();
    class State *acceptState = createState(true);

    if (inputChar == ' ')
    {
        addTransition(startState, acceptState, '\0'); // 使用 '\0' 代表空转换
    }
    else
    {
        addTransition(startState, acceptState, inputChar);
    }

    return new class NFA(startState, acceptState);
}

class NFA *concatenate(class NFA *nfa1, class NFA *nfa2)
{
    // 将nfa2的开始状态的所有转换添加到nfa1的接受状态
    for (auto &transition : nfa2->start->transitions)
    {
        nfa1->accept->transitions.push_back(transition);
    }
    // 清除nfa2的开始状态的所有转换
    nfa2->start->transitions.clear();

    // 设置nfa1的接受状态为非终止状态
    nfa1->accept->isFinal = false;

    // 返回新的串联NFA
    return new class NFA(nfa1->start, nfa2->accept);
}

class NFA *alternate(class NFA *nfa1, class NFA *nfa2)
{
    class State *startState = createState();
    class State *acceptState = createState(true);

    addTransition(startState, nfa1->start, '\0');
    addTransition(startState, nfa2->start, '\0');
    addTransition(nfa1->accept, acceptState, '\0');
    addTransition(nfa2->accept, acceptState, '\0');
    nfa1->accept->isFinal = false;
    nfa2->accept->isFinal = false;

    return new class NFA(startState, acceptState);
}

class NFA *kleeneStar(class NFA *nfa)
{
    class State *startState = createState();
    class State *acceptState = createState(true);

    addTransition(startState, acceptState, '\0');
    addTransition(startState, nfa->start, '\0');
    addTransition(nfa->accept, acceptState, '\0');
    addTransition(nfa->accept, nfa->start, '\0');
    nfa->accept->isFinal = false;

    return new class NFA(startState, acceptState);
}

std::string infixToPostfix(const std::string &regex)
{
    std::stack<char> stack;
    std::string postfix = "";
    std::string modifiedRegex = "";

    // 插入串联操作符
    for (size_t i = 0; i < regex.size() - 1; ++i)
    {
        modifiedRegex += regex[i];
        if ((std::isalpha(regex[i]) || regex[i] == '*' || regex[i] == ')') && (std::isalpha(regex[i + 1]) || regex[i + 1] == '('))
        {
            modifiedRegex += '.';
        }
    }
    modifiedRegex += regex.back();

    for (char c : modifiedRegex)
    {
        if (std::isalpha(c) || c == ' ') // 判断字符是否为字母
        {
            postfix += c;
        }
        else if (c == '(')
        {
            stack.push(c);
        }
        else if (c == ')')
        {
            while (stack.top() != '(')
            {
                postfix += stack.top();
                stack.pop();
            }
            stack.pop();
        }
        else if (c == '*' || c == '|' || c == '.')
        {
            // 调整操作符的优先级
            while (!stack.empty() && stack.top() != '(' &&
                   ((c == '*' && stack.top() == '*') ||
                    (c == '.' && (stack.top() == '.' || stack.top() == '*')) ||
                    (c == '|' && (stack.top() == '.' || stack.top() == '*' || stack.top() == '|'))))
            {
                postfix += stack.top();
                stack.pop();
            }
            stack.push(c);
        }
    }

    while (!stack.empty())
    {
        postfix += stack.top();
        stack.pop();
    }

    return postfix;
}

void generateDotFile(class NFA *nfa, const std::string &filename)
{
    std::ofstream outfile(filename);

    if (outfile.is_open())
    {
        outfile << "digraph NFA {\n";
        outfile << "  rankdir=LR;\n";
        outfile << "  node [shape = circle];\n";

        std::stack<class State *> stack;
        std::vector<int> visitedStates;
        stack.push(nfa->start);

        while (!stack.empty())
        {
            class State *currentState = stack.top();
            stack.pop();

            if (std::find(visitedStates.begin(), visitedStates.end(), currentState->id) != visitedStates.end())
                continue;
            visitedStates.push_back(currentState->id);

            if (currentState->isFinal)
            {
                outfile << "  \""
                        << "S" << currentState->id << "\" [shape = doublecircle];\n";
            }
            else
            {
                outfile << "  \""
                        << "S" << currentState->id << "\" [shape = circle];\n";
            }

            for (const auto &transition : currentState->transitions)
            {
                outfile << "  \"S" << currentState->id << "\" -> \"S" << transition.target->id << "\" [label=\"" << (transition.symbol == '\0' ? "ε" : std::string(1, transition.symbol)) << "\"];\n";

                stack.push(transition.target);
            }
        }

        outfile << "}\n";
        outfile.close();
        std::cout << "NFA已生成到 " << filename << " 文件中\n";
    }
    else
    {
        std::cerr << "无法打开文件以写入输出\n";
    }
}

NFA *generateThompsonNFAFromPostfix(const std::string &postfix)
{
    std::stack<NFA *> nfaStack;

    for (char c : postfix)
    {
        if (isalpha(c) || c == ' ') // 使用C++中的isalpha函数检查字符是否为字母
        {
            nfaStack.push(thompsonConstruction(c));
        }
        else if (c == '|')
        {
            NFA *nfa2 = nfaStack.top();
            nfaStack.pop();
            NFA *nfa1 = nfaStack.top();
            nfaStack.pop();
            nfaStack.push(alternate(nfa1, nfa2));
        }
        else if (c == '*')
        {
            NFA *nfa = nfaStack.top();
            nfaStack.pop();
            nfaStack.push(kleeneStar(nfa));
        }
        else if (c == '.')
        {
            NFA *nfa2 = nfaStack.top();
            nfaStack.pop();
            NFA *nfa1 = nfaStack.top();
            nfaStack.pop();
            nfaStack.push(concatenate(nfa1, nfa2));
        }
    }

    return nfaStack.top();
}

// DFA子集构造
struct DFAState
{
    int id;
    bool isFinal;
    std::set<State *> nfaStates;
    std::map<char, DFAState *> transitions;
    DFAState(int _id) : id(_id), isFinal(false) {}
};

std::vector<DFAState *> dfaStates;
std::map<std::set<State *>, int> stateMap;
int getOrCreateDFAState(const std::set<State *> &nfaStateSet)
{
    // 输出正在处理的NFA状态集
    std::cout << "Checking or creating DFA state for NFA states: ";
    for (State *s : nfaStateSet)
    {
        std::cout << s->id << " ";
    }
    std::cout << std::endl;

    if (stateMap.find(nfaStateSet) == stateMap.end())
    {
        DFAState *newState = new DFAState(dfaStates.size());

        for (State *s : nfaStateSet)
        {
            if (s->isFinal)
            {
                newState->isFinal = true;
                break;
            }
        }
        newState->nfaStates = nfaStateSet;
        dfaStates.push_back(newState);
        stateMap[nfaStateSet] = newState->id;

        // 输出已经为NFA状态集创建了新的DFA状态的信息
        std::cout << "Created new DFA state " << newState->id << " for NFA states: ";
        for (State *s : nfaStateSet)
        {
            std::cout << s->id << " ";
        }
        std::cout << std::endl;
    }
    else
    {
        // 输出已经为NFA状态集找到了现有的DFA状态的信息
        std::cout << "Found existing DFA state " << stateMap[nfaStateSet] << " for NFA states: ";
        for (State *s : nfaStateSet)
        {
            std::cout << s->id << " ";
        }
        std::cout << std::endl;
    }

    return stateMap[nfaStateSet];
}

std::set<State *> eClosure(State *state)
{
    std::set<State *> result;
    std::stack<State *> stack;
    stack.push(state);

    while (!stack.empty())
    {
        State *current = stack.top();
        stack.pop();

        if (result.find(current) != result.end())
            continue;
        result.insert(current);

        for (Transition t : current->transitions)
        {
            if (t.symbol == '\0' && result.find(t.target) == result.end())
            {
                stack.push(t.target);
            }
        }
    }
    return result;
}

std::set<State *> eClosure(const std::set<State *> &stateSet)
{
    std::set<State *> result;
    for (State *s : stateSet)
    {
        std::set<State *> temp = eClosure(s);
        result.insert(temp.begin(), temp.end());
    }

    // Debug output
    std::cout << "eClosure of states: ";
    for (State *s : stateSet)
    {
        std::cout << s->id << " ";
    }
    std::cout << "results in states: ";
    for (State *s : result)
    {
        std::cout << s->id << " ";
    }
    std::cout << std::endl;

    return result;
}

std::set<State *> move(const std::set<State *> &stateSet, char symbol)
{
    std::set<State *> result;
    for (State *s : stateSet)
    {
        for (Transition t : s->transitions)
        {
            if (t.symbol == symbol)
            {
                result.insert(t.target);
            }
        }
    }

    // Debug output
    std::cout << "Moving with symbol: " << symbol << " from states: ";
    for (State *s : stateSet)
    {
        std::cout << s->id << " ";
    }
    std::cout << "to states: ";
    for (State *s : result)
    {
        std::cout << s->id << " ";
    }
    std::cout << std::endl;

    return result;
}

void constructDFAFromNFA(NFA *nfa, const std::set<State *> &nfaStates)
{
    std::set<State *> startStateSet = eClosure(nfa->start);
    std::queue<std::set<State *>> processQueue;
    processQueue.push(startStateSet);
    getOrCreateDFAState(startStateSet);

    std::set<char> inputSymbols;
    for (State *s : nfaStates)
    {
        for (Transition t : s->transitions)
        {
            if (t.symbol != '\0')
            {
                inputSymbols.insert(t.symbol);
            }
        }
    }

    while (!processQueue.empty())
    {
        std::set<State *> currentStateSet = processQueue.front();
        processQueue.pop();
        DFAState *currentDFAState = dfaStates[getOrCreateDFAState(currentStateSet)];

        // 输出当前正在处理的DFA状态
        std::cout << "Processing DFA state: ";
        for (State *s : currentStateSet)
        {
            std::cout << s->id << " "; // 假设State有一个名为"name"的成员，表示状态的名称
        }
        std::cout << std::endl;

        for (char symbol : inputSymbols)
        {
            std::set<State *> nextStateSet = eClosure(move(currentStateSet, symbol));

            // 输出对应于给定符号的转移的结果状态集合
            std::cout << "Moving with symbol " << symbol << " results in states: ";
            for (State *s : nextStateSet)
            {
                std::cout << s->id << " ";
            }
            std::cout << std::endl;

            if (!nextStateSet.empty())
            {

                if (stateMap.find(nextStateSet) == stateMap.end()) // Check if this DFA state hasn't been processed yet
                {
                    processQueue.push(nextStateSet);
                }
                int nextStateId = getOrCreateDFAState(nextStateSet); // Always get or create DFA state
                currentDFAState->transitions[symbol] = dfaStates[nextStateId];
            }
        }
    }
}

std::set<State *> collectStatesFromNFA(NFA *nfa)
{
    std::set<State *> states;
    std::stack<State *> stack;
    stack.push(nfa->start);

    while (!stack.empty())
    {
        State *curr = stack.top();
        stack.pop();

        std::cout << "Processing state: S" << curr->id << std::endl; // 输出当前处理的状态

        if (states.find(curr) == states.end())
        {
            states.insert(curr);
            std::cout << "Inserted state: S" << curr->id << " to states set. Total states: " << states.size() << std::endl; // 输出状态集合大小

            for (const auto &trans : curr->transitions)
            {
                std::cout << "Transition from S" << curr->id << " to S" << trans.target->id << " with label: " << (trans.symbol == '\0' ? "ε" : std::string(1, trans.symbol)) << std::endl; // 输出转移信息
                stack.push(trans.target);
            }
        }
    }
    return states;
}

void generateDotFileForDFA(const std::string &filename)
{
    std::ofstream outfile(filename);

    if (outfile.is_open())
    {
        outfile << "digraph DFA {\n";
        outfile << "  rankdir=LR;\n";
        outfile << "  node [shape = circle];\n";

        for (DFAState *dfaState : dfaStates)
        {
            // 我们将使用NFA状态的集合作为DFA状态的名字
            std::string stateName = "{";
            for (State *nfaState : dfaState->nfaStates)
            {
                stateName += "S" + std::to_string(nfaState->id) + ",";
            }
            stateName.back() = '}'; // 替换最后的逗号

            if (dfaState->isFinal)
            {
                outfile << "  \"" << stateName << "\" [shape = doublecircle];\n";
            }
            else
            {
                outfile << "  \"" << stateName << "\" [shape = circle];\n";
            }

            for (const auto &transition : dfaState->transitions)
            {
                std::string targetName = "{";
                for (State *nfaState : transition.second->nfaStates)
                {
                    targetName += "S" + std::to_string(nfaState->id) + ",";
                }
                targetName.back() = '}';

                outfile << "  \"" << stateName << "\" -> \"" << targetName << "\" [label=\"" << transition.first << "\"];\n";
            }
        }

        outfile << "}\n";
        outfile.close();
        std::cout << "DFA已生成到 " << filename << " 文件中\n";
    }
    else
    {
        std::cerr << "无法打开文件以写入输出\n";
    }
}
// 最小化
// DFA 最小化
void minimizeDFA()
{
    // 初始化
    std::vector<std::set<DFAState *>> partitions;
    std::set<DFAState *> accepting, nonAccepting;

    for (DFAState *state : dfaStates)
    {
        if (state->isFinal)
        {
            accepting.insert(state);
        }
        else
        {
            nonAccepting.insert(state);
        }
    }

    partitions.push_back(accepting);
    partitions.push_back(nonAccepting);

    std::vector<std::set<DFAState *>> newPartitions;
    bool partitioned = true;

    while (partitioned)
    {
        partitioned = false;
        newPartitions.clear();

        for (const auto &part : partitions)
        {
            std::map<std::string, std::set<DFAState *>> splitSets;

            for (DFAState *state : part)
            {
                std::string signature = "";
                for (const auto &[symbol, targetState] : state->transitions)
                {
                    for (size_t i = 0; i < partitions.size(); ++i)
                    {
                        if (partitions[i].find(targetState) != partitions[i].end())
                        {
                            signature += std::to_string(i) + symbol;
                            break;
                        }
                    }
                }
                splitSets[signature].insert(state);
            }

            for (const auto &[_, splitSet] : splitSets)
            {
                newPartitions.push_back(splitSet);
            }
        }

        if (newPartitions.size() != partitions.size())
        {
            partitioned = true;
            partitions = newPartitions;
        }
    }

    // 创建新的DFA状态
    std::vector<DFAState *> newDFAStates;
    for (const auto &part : partitions)
    {
        if (!part.empty())
        {
            DFAState *newState = new DFAState(newDFAStates.size());
            newDFAStates.push_back(newState);
            std::cout << "Creating new state with id: " << newState->id << std::endl; // 调试输出
        }
    }

    // 设置转换
    for (size_t index = 0; index < partitions.size(); ++index)
    {
        const auto &part = partitions[index];
        DFAState *newState = newDFAStates[index];
        DFAState *representative = *part.begin();
        newState->isFinal = representative->isFinal;

        for (const auto &[symbol, targetState] : representative->transitions)
        {
            for (size_t i = 0; i < partitions.size(); ++i)
            {
                if (partitions[i].find(targetState) != partitions[i].end())
                {
                    newState->transitions[symbol] = newDFAStates[i];
                    std::cout << "Setting transition: " << symbol << " -> State " << newDFAStates[i]->id << std::endl; // 调试输出
                    break;
                }
            }
        }
    }

    // 释放原始DFA状态的内存
    for (DFAState *state : dfaStates)
    {
        delete state;
    }

    // 更新DFA状态列表
    dfaStates = newDFAStates;
}

void generateMinimizedDotFileForDFA(const std::string &filename)
{
    std::ofstream outfile(filename);

    if (outfile.is_open())
    {
        outfile << "digraph MinimizedDFA {\n";
        outfile << "  rankdir=LR;\n";
        outfile << "  node [shape = circle];\n";

        for (DFAState *dfaState : dfaStates)
        {
            std::string stateName = "S" + std::to_string(dfaState->id);
            std::cout << "Processing state with id: " << dfaState->id << std::endl; // 调试输出
            if (dfaState->isFinal)
            {
                outfile << "  \"" << stateName << "\" [shape = doublecircle];\n";
            }
            else
            {
                outfile << "  \"" << stateName << "\" [shape = circle];\n";
            }

            for (const auto &transition : dfaState->transitions)
            {
                if (!transition.second)
                {
                    std::cerr << "Error: Invalid pointer for target DFA state." << std::endl;
                    continue; // Skip this transition
                }

                std::cout << "Transition: " << transition.first << " -> State " << transition.second->id << std::endl; // 调试输出

                std::string targetName = "S" + std::to_string(transition.second->id);
                outfile << "  \"" << stateName << "\" -> \"" << targetName << "\" [label=\"" << transition.first << "\"];\n";
            }
        }

        outfile << "}\n";
        outfile.close();
        std::cout << "Minimized DFA has been generated to " << filename << "\n";
    }
    else
    {
        std::cerr << "Unable to open file for writing output\n";
    }
}

int main()
{

    std::string regex = "a|(b|c|e) | |d*";
    std::string postfix = infixToPostfix(regex);
    std::cout << "后缀表达式: " << postfix << std::endl;
    NFA *finalNFA = generateThompsonNFAFromPostfix(postfix);
    generateDotFile(finalNFA, "thompson_nfa.dot");

    std::set<State *> nfaStates = collectStatesFromNFA(finalNFA);

    constructDFAFromNFA(finalNFA, nfaStates);
    generateDotFileForDFA("dfa_output.dot");
    minimizeDFA();
    generateMinimizedDotFileForDFA("minimized_dfa_output.dot");

    return 0;
}
