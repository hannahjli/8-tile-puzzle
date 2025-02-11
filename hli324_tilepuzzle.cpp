/* This program was written by Hannah Li, for 
the UC Riverside course CS170 - Intro to AI.
The provided general search function serves as a base
for all 3 search functions in this file. 

This pseduocode was obtained from CS170 materials 
written by Professor Eammon Keogh:

function general-search(problem, QUEUEING-FUNCTION)
nodes = MAKE-QUEUE(MAKE-NODE(problem.INITIAL-STATE))
loop do
if EMPTY(nodes) then return "failure"
node = REMOVE-FRONT(nodes)
if problem.GOAL-TEST(node.STATE) succeeds then return node
nodes = QUEUEING-FUNCTION(nodes, EXPAND(node, problem.OPERATORS))
end
*/

#include <queue>
#include <vector>
#include <cmath> //used for sqrt()
#include <iostream> //used for cin cout
#include <iomanip> //used for setw() to make larger puzzles align

using namespace std;

int TILE_NUM = 8; //can be changed for other valid tile puzzle sizes (i.e., 3,15,24)
//this has NOT been tested for any other puzzle size besides 8
int GRID_SIZE = (TILE_NUM+1);
int GRID_EDGE = sqrt(GRID_SIZE);
vector<int> initial_puzzle(GRID_SIZE); 

// some tester puzzles for user
vector<int> easy8 = {1,2,0,5,7,3,4,8,6};
vector<int> medium8 = {7,3,6,2,1,8,5,4,0};
vector<int> hard8 = {3,6,0,4,2,5,7,8,1};

struct Node { //Node represents the state of the puzzle with additional info listed accordingly
    public:
        vector<int> puzzle; //current puzzle vector 
        int blank_index; // blank tile index 
        int g_n; // path cost g(n)
        int h_n; //hueristic cost h(n) calculated by hueristic functions
        int depth; //depth of current state
        Node* parent; //pointer to parent (for tracing solution path)

        Node();
        Node(vector<int> p, int b, int g, int h, int d, Node* par);

        ~Node() {
            delete parent;
        }
};

vector<Node*> allocated_nodes; //used for clean up after search
void clean() {
    for (Node* node : allocated_nodes) {
        delete node;
    }
    allocated_nodes.clear();
}

Node::Node() {}
Node::Node(vector<int> p, int b, int g, int h, int d, Node* par) {
    puzzle = p;
    blank_index = b;
    g_n = g;
    h_n = h;
    depth = d;
    parent = par;
}

void set_difficulty(int num) {
    switch (num)
    {
    case 1:
        initial_puzzle = easy8;
        cout << "Difficulty " << num << ": easy selected. Initial state is:" << endl;
        break;
    case 2:
        initial_puzzle = medium8;
        cout << "Difficulty " << num << ": medium selected. Initial state is:" << endl;
        break;
    case 3:
        initial_puzzle = hard8;
        cout << "Difficulty " << num << ": hard selected. Initial state is:" << endl;
        break;
    
    default:
        cout << "Invalid entry or selection error, defaulting to medium difficulty." << endl;
        initial_puzzle = medium8;
        break;
    }
}

const vector<int> defaultSolved() {
    vector<int> goalState(GRID_SIZE);

    for (int i = 0; i < GRID_SIZE - 1; i++) {
        goalState.at(i) = i+1;
    }
    goalState.at(GRID_SIZE-1) = 0;

    return goalState;
}
const vector<int> goalstate = defaultSolved(); //generates the solved puzzle vector aka goal state

void outputPuzzle(vector<int> puzzle) { //prints out a puzzle
    for (int i = 0; i < GRID_EDGE; i++) {
        cout << "[";
        for (int j = 0; j < GRID_EDGE; j++) {
            cout << setw(3) << puzzle.at((GRID_EDGE*i)+j) << " ";
        }
        cout << " ]" << endl;
    } 
}

vector<int> possibleMovesFromB(int b_index) { //creates a vector of legal indexes that 0 can be moved to
    //essentially, compiles all possible blank_index values
    vector<int> valid_indexes;

    int row_num = b_index/GRID_EDGE; //columns and rows (x,y) coords where range is 0-2 for 8 puzzle
    int col_num = b_index%GRID_EDGE; //since i use a 1D array, i have to calculate the coords
    //for the 8 puzzle example, if 0 is at index 4, {1,2,3,4,0,5,6,7,8} these are coords 1,1
    //the row is 4/3 rounded to 1
    //the column is 4%3 = 1
    //so, valid moves are up to (1,0), down to (1,2), left to (0,1), or right to (2,1)
    //the example positions vs the correct positions are: 
    // 1(0,0) 2(1,0) 3(2,0)         1(0,0) 2(1,0) 3(2,0)
    // 5(0,1) 0(1,1) 6(2,1)         4(0,1) 5(1,1) 6(2,1)
    // 4(0,2) 7(1,2) 8(2,2)         7(0,2) 8(1,2) 0(2,2)

    if (row_num > 0) {  //slide up
        valid_indexes.push_back(b_index - GRID_EDGE); //pushes index 1 for example
    }
    if (row_num < GRID_EDGE-1) { //slide down
        valid_indexes.push_back(b_index + GRID_EDGE); //pushes 7 for example
    }
    if (col_num > 0) { //slide left
        valid_indexes.push_back(b_index - 1); //pushes 3 for example
    }
    if (col_num < GRID_EDGE-1) { //slide right
        valid_indexes.push_back(b_index + 1); //pushes 5 for example
    }

    return valid_indexes;
}

//Node functions
Node* newNode(vector<int> puzz, int curr_b, int new_b, Node* par, int h_cost) { 
    //creates a new state representation
    //inherit puzzle vector and parent pointer
    Node* new_state = new Node;
    new_state->parent = par;
    new_state->puzzle = puzz;
    
    //swap blank with new blank location
    swap(new_state->puzzle.at(curr_b), new_state->puzzle.at(new_b));
    new_state->blank_index = new_b;

    //update depth and path costs based on the parent's depth and cost
    if (par != nullptr) {
        new_state->depth = par->depth + 1;
        new_state->g_n = par->g_n + 1;
    }
    else {
        new_state->depth = 0; //if parent is null, then this is the root at depth 0
        new_state->g_n = 0; //samething, cost at root is 0 (nothing moved yet)
    }

    new_state->h_n = h_cost; //in uniform, this is always 0
    //otherwise use the hueristic functions to find h_cost

    return new_state;
}

//2 functions for calculating h_n: misplaced tile and manhattan distance
int misplaced_h(vector<int> &puzzle) {
    int misplaced = 0;
    for (int i = 0; i < puzzle.size(); i++) {
        if (puzzle.at(i) != i + 1) {  // Ignore blank tile and count misplaced tiles
            misplaced++;
        }
    }
    
    return misplaced - 1;
}
int manhattan_h(vector<int> &puzzle) {
    int dist = 0;
    for (int i = 0; i < puzzle.size(); i++) {
        if(puzzle.at(i) != 0) {
            int curr_row = i/GRID_EDGE;  //position of current index
            int curr_col = i%GRID_EDGE;
            int goal_row = (puzzle.at(i)-1) / GRID_EDGE; //position of where the value is supposed to be
            int goal_col = (puzzle.at(i)-1) % GRID_EDGE;
        
            dist += abs(curr_col - goal_col) + abs(curr_row - goal_row);
        }
    }
    return dist;
}

//find b_index of puzzle
int blank_at(vector<int> p) {
    for (int i = 0; i < p.size(); i++) {
        if (p.at(i) == 0) {
            return i;
        }
    }
    return -1;
}

vector<Node*> expand(Node* n, int(*calculate_h)(vector<int>&)) {
    vector<Node*> children;
    vector<int> possible_moves = possibleMovesFromB(n->blank_index);

    for (int i = 0; i < possible_moves.size(); i++) {
        int new_blank = possible_moves.at(i);
        int h_cost = 0;        

        Node* child = newNode(n->puzzle, n->blank_index, new_blank, n, h_cost);
        //newNode takes care of swapping tiles, so n->puzzle and child->puzzle are different

        if (calculate_h != nullptr) { //calculate and update h(n) if not uniform cost
            h_cost = calculate_h(child->puzzle);
            child->h_n = h_cost; 
        }

        bool duplicate_found = false;
        //node n is now child's parent
        for (Node* ptr = n; ptr != nullptr; ptr = ptr->parent) {
            if (child->puzzle == ptr->puzzle) {
                duplicate_found = true;
                break;
            }
        }

        if (duplicate_found == false) {
            children.push_back(child);
            allocated_nodes.push_back(child);
        } 
    }

    return children;
}

Node* initial_state_node(vector<int> p) { //constructs root
    p = initial_puzzle;
    int init_b = blank_at(p);
    Node* root = new Node(p, init_b, 0, 0, 0, nullptr);

    //these are mostly sanity checks
    cout << "Blank tile is at index: " << init_b << endl;
    cout << "Cost g(n) = " << root->g_n << endl;
    cout << "Hueristic cost h(n) = " << root->h_n << endl;
    cout << "Depth d = " << root->depth << endl;
    if (root->parent == nullptr) {
        cout << "Parent is nullptr. This is the root." << endl;
    }

    return root;
}
//i use the root from initial_state_node as the parameter for the searches

//general search algos (combine everything, account for different queueing function choices)
Node* uni_cost_search(Node* problem) {
    //only comparison is g(n), because h(n) = 0 for uniform
    struct uniform_cost {
        bool operator() (Node* a, Node* b) {
            return a->g_n  >  b->g_n; //prioritizes lower cost of g(n)
        }
    };

    priority_queue<Node*, vector<Node*>, uniform_cost> nodes;
    nodes.push(problem);
    int nodes_expanded = 0;

    while (!nodes.empty()) {
        Node* node = nodes.top();
        nodes.pop();
        outputPuzzle(node->puzzle);
        cout << "depth = " << node->depth << endl;
        cout << "g(n) = " << node->g_n << " , h(n) = " << node->h_n << endl;
        cout << "f(n) = g(n) + h(n) = " << node->g_n + node->h_n << endl;
        cout << endl << endl;

        if (node->puzzle == goalstate) {
            cout << "Solution found at depth: d = " << node->depth << endl;
            cout << "Nodes expanded: " << nodes_expanded << endl;
            return node;
        }

        nodes_expanded++;

        vector<Node*> children = expand(node, nullptr);

        for (int i = 0; i < children.size(); i++) {
                nodes.push(children.at(i));
        }
        cout << "size of nodes pqueue: " << nodes.size() << endl;
    }

    cout << "failure" << endl;
    clean();
    return nullptr;
}

Node* Astar_misplaced_search(Node* problem) {
    //comparison to use for A* because we need g(n) + h(n) now
    struct Astar_cost {
        public:
        bool operator() (Node* a, Node* b) {
            return (a->g_n + a->h_n) > (b->g_n + b->h_n);
        }
    };

    priority_queue<Node*, vector<Node*>, Astar_cost> nodes;
    nodes.push(problem);
    int nodes_expanded = 0;

    while (!nodes.empty()) {
        Node* node = nodes.top();
        nodes.pop();
        outputPuzzle(node->puzzle);
        cout << "depth = " << node->depth << endl;
        cout << "g(n) = " << node->g_n << " , h(n) = " << node->h_n << endl;
        cout << "f(n) = g(n) + h(n) = " << node->g_n + node->h_n << endl;
        cout << endl << endl;

        if (node->puzzle == goalstate) {
            cout << "Solution found at depth: d = " << node->depth << endl;
            cout << "Nodes expanded: " << nodes_expanded << endl;
            return node;
        }

        nodes_expanded++;
        
        vector<Node*> children = expand(node, misplaced_h);

        for (int i = 0; i < children.size(); i++) {
            nodes.push(children.at(i));
        }
        cout << "size of nodes pqueue: " << nodes.size() << endl;
    }

    cout << "failure" << endl;
    clean();
    return nullptr;
}

Node* Astar_manhattan_search(Node* problem) {
    //comparison to use for A* because we need g(n) + h(n) now
    struct Astar_cost {
        public:
        bool operator() (Node* a, Node* b) {
            return (a->g_n + a->h_n) > (b->g_n + b->h_n);
        }
    };

    priority_queue<Node*, vector<Node*>, Astar_cost> nodes;
    nodes.push(problem);
    int nodes_expanded = 0;

    while (!nodes.empty()) {
        Node* node = nodes.top();
        nodes.pop();
        outputPuzzle(node->puzzle);
        cout << "depth = " << node->depth << endl;
        cout << "g(n) = " << node->g_n << " , h(n) = " << node->h_n << endl;
        cout << "f(n) = g(n) + h(n) = " << node->g_n + node->h_n << endl;
        cout << endl << endl;

        if (node->puzzle == goalstate) {
            cout << "Solution found at depth: d = " << node->depth << endl;
            cout << "Nodes expanded: " << nodes_expanded << endl;
            return node;
        }

        nodes_expanded++;
        
        vector<Node*> children = expand(node, manhattan_h);

        for (int i = 0; i < children.size(); i++) {
            nodes.push(children.at(i));
        }
        cout << "size of nodes pqueue: " << nodes.size() << endl;
    }

    cout << "failure" << endl;
    clean();
    return nullptr;
}

int main() {
    
    cout << "Testing solving for 8 Puzzle with easy, medium, and hard shuffles." << endl;
    cout << endl;
    cout << "To begin, enter the corresponding number option:" << endl;
    cout << "1 for easy puzzle" << endl;
    cout << "2 for medium puzzle" << endl;
    cout << "3 for hard puzzle" << endl;
    cout << endl;

    int difficulty = 0;
    cin >> difficulty;

    set_difficulty(difficulty); //sets initialpuzzle to 1 of 3 puzzles
    outputPuzzle(initial_puzzle); //print to check

    cout << "Select algorithm by entering the corresponding number option:" << endl;
    cout << "1 for Uniform Cost Search" << endl;
    cout << "2 for A* with Misplaced Tile heuristic" << endl;
    cout << "3 for A* with Manhattan Distance heuristic" << endl;
    cout << endl;

    int algo = 0;
    cin >> algo;
    Node* root = initial_state_node(initial_puzzle);
    Node* solution;
    
    switch (algo)
    {
    case 1:
        cout << algo << ": Uniform Cost Search selected." << endl;
        solution = uni_cost_search(root);
        break;
    case 2:
        cout << algo << ": A* with Misplaced Tile heuristic selected." << endl;
        solution = Astar_misplaced_search(root);
        break;
    case 3:
        cout << algo << ": A* with Manhattan Distance heuristic selected." << endl;
        solution = Astar_manhattan_search(root);
        break;
    default:
        cout << "Invalid entry or selection error, defaulting to A* with Misplaced Tile." << endl;
        algo = 2;
        break;
    }

    return 0;
}
