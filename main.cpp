#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace std;

const int MAX_KEY_LEN = 65;
const int BLOCK_SIZE = 4096;
const int MAX_CHILDREN = 100;
const int MIN_CHILDREN = 50;

struct KeyValue {
    char key[MAX_KEY_LEN];
    int value;
    
    KeyValue() {
        memset(key, 0, sizeof(key));
        value = 0;
    }
    
    KeyValue(const char* k, int v) {
        strncpy(key, k, MAX_KEY_LEN - 1);
        key[MAX_KEY_LEN - 1] = '\0';
        value = v;
    }
    
    bool operator<(const KeyValue& other) const {
        int cmp = strcmp(key, other.key);
        if (cmp != 0) return cmp < 0;
        return value < other.value;
    }
    
    bool operator==(const KeyValue& other) const {
        return strcmp(key, other.key) == 0 && value == other.value;
    }
};

struct Node {
    bool is_leaf;
    int count;
    KeyValue keys[MAX_CHILDREN];
    int children[MAX_CHILDREN + 1];
    int next_leaf;
    
    Node() : is_leaf(true), count(0), next_leaf(-1) {
        memset(children, -1, sizeof(children));
    }
};

class BPlusTree {
private:
    string filename;
    int root_pos;
    int node_count;
    
    Node read_node(int pos) {
        Node node;
        ifstream file(filename, ios::binary);
        if (file) {
            file.seekg(sizeof(int) * 2 + pos * sizeof(Node));
            file.read((char*)&node, sizeof(Node));
            file.close();
        }
        return node;
    }
    
    void write_node(int pos, const Node& node) {
        fstream file(filename, ios::binary | ios::in | ios::out);
        if (!file) {
            file.open(filename, ios::binary | ios::out);
        }
        file.seekp(sizeof(int) * 2 + pos * sizeof(Node));
        file.write((char*)&node, sizeof(Node));
        file.close();
    }
    
    void write_header() {
        fstream file(filename, ios::binary | ios::in | ios::out);
        if (!file) {
            file.open(filename, ios::binary | ios::out);
        }
        file.seekp(0);
        file.write((char*)&root_pos, sizeof(int));
        file.write((char*)&node_count, sizeof(int));
        file.close();
    }
    
    void read_header() {
        ifstream file(filename, ios::binary);
        if (file) {
            file.read((char*)&root_pos, sizeof(int));
            file.read((char*)&node_count, sizeof(int));
            file.close();
        } else {
            root_pos = 0;
            node_count = 1;
            Node root;
            root.is_leaf = true;
            write_node(0, root);
            write_header();
        }
    }
    
    int allocate_node() {
        return node_count++;
    }
    
    void split_child(int parent_pos, int child_idx) {
        Node parent = read_node(parent_pos);
        int child_pos = parent.children[child_idx];
        Node child = read_node(child_pos);
        
        int new_pos = allocate_node();
        Node new_node;
        new_node.is_leaf = child.is_leaf;
        new_node.count = MIN_CHILDREN;
        
        for (int i = 0; i < MIN_CHILDREN; i++) {
            new_node.keys[i] = child.keys[i + MIN_CHILDREN];
        }
        
        if (!child.is_leaf) {
            for (int i = 0; i <= MIN_CHILDREN; i++) {
                new_node.children[i] = child.children[i + MIN_CHILDREN];
            }
        } else {
            new_node.next_leaf = child.next_leaf;
            child.next_leaf = new_pos;
        }
        
        child.count = MIN_CHILDREN;
        
        for (int i = parent.count; i > child_idx; i--) {
            parent.children[i + 1] = parent.children[i];
            parent.keys[i] = parent.keys[i - 1];
        }
        
        parent.children[child_idx + 1] = new_pos;
        parent.keys[child_idx] = new_node.keys[0];
        parent.count++;
        
        write_node(child_pos, child);
        write_node(new_pos, new_node);
        write_node(parent_pos, parent);
    }
    
    void insert_non_full(int pos, const KeyValue& kv) {
        Node node = read_node(pos);
        
        if (node.is_leaf) {
            int i = node.count - 1;
            while (i >= 0 && kv < node.keys[i]) {
                node.keys[i + 1] = node.keys[i];
                i--;
            }
            node.keys[i + 1] = kv;
            node.count++;
            write_node(pos, node);
        } else {
            int i = node.count - 1;
            while (i >= 0 && kv < node.keys[i]) {
                i--;
            }
            i++;
            
            Node child = read_node(node.children[i]);
            if (child.count == MAX_CHILDREN) {
                split_child(pos, i);
                node = read_node(pos);
                if (node.keys[i] < kv) {
                    i++;
                }
            }
            insert_non_full(node.children[i], kv);
        }
    }
    
public:
    BPlusTree(const string& fname) : filename(fname) {
        read_header();
    }
    
    ~BPlusTree() {
        write_header();
    }
    
    void insert(const char* key, int value) {
        KeyValue kv(key, value);
        
        Node root = read_node(root_pos);
        if (root.count == MAX_CHILDREN) {
            int new_root_pos = allocate_node();
            Node new_root;
            new_root.is_leaf = false;
            new_root.count = 0;
            new_root.children[0] = root_pos;
            write_node(new_root_pos, new_root);
            
            split_child(new_root_pos, 0);
            root_pos = new_root_pos;
            insert_non_full(root_pos, kv);
        } else {
            insert_non_full(root_pos, kv);
        }
        write_header();
    }
    
    void find(const char* key, vector<int>& results) {
        results.clear();
        Node node = read_node(root_pos);
        
        while (!node.is_leaf) {
            int i = 0;
            while (i < node.count && strcmp(key, node.keys[i].key) >= 0) {
                i++;
            }
            node = read_node(node.children[i]);
        }
        
        while (true) {
            for (int i = 0; i < node.count; i++) {
                if (strcmp(node.keys[i].key, key) == 0) {
                    results.push_back(node.keys[i].value);
                } else if (strcmp(node.keys[i].key, key) > 0) {
                    sort(results.begin(), results.end());
                    return;
                }
            }
            
            if (node.next_leaf == -1) break;
            node = read_node(node.next_leaf);
        }
        
        sort(results.begin(), results.end());
    }
    
    void remove(const char* key, int value) {
        // Simple implementation: find and mark as deleted
        // For a full implementation, we would need to handle merging and rebalancing
        KeyValue target(key, value);
        remove_from_node(root_pos, target);
        write_header();
    }
    
    void remove_from_node(int pos, const KeyValue& kv) {
        Node node = read_node(pos);
        
        if (node.is_leaf) {
            int i = 0;
            while (i < node.count && !(node.keys[i] == kv)) {
                i++;
            }
            if (i < node.count) {
                for (int j = i; j < node.count - 1; j++) {
                    node.keys[j] = node.keys[j + 1];
                }
                node.count--;
                write_node(pos, node);
            }
        } else {
            int i = 0;
            while (i < node.count && kv < node.keys[i]) {
                i++;
            }
            remove_from_node(node.children[i], kv);
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(0);
    
    BPlusTree tree("bptree.dat");
    
    int n;
    cin >> n;
    
    for (int i = 0; i < n; i++) {
        string cmd;
        cin >> cmd;
        
        if (cmd == "insert") {
            char key[MAX_KEY_LEN];
            int value;
            cin >> key >> value;
            tree.insert(key, value);
        } else if (cmd == "delete") {
            char key[MAX_KEY_LEN];
            int value;
            cin >> key >> value;
            tree.remove(key, value);
        } else if (cmd == "find") {
            char key[MAX_KEY_LEN];
            cin >> key;
            vector<int> results;
            tree.find(key, results);
            if (results.empty()) {
                cout << "null\n";
            } else {
                for (size_t j = 0; j < results.size(); j++) {
                    if (j > 0) cout << " ";
                    cout << results[j];
                }
                cout << "\n";
            }
        }
    }
    
    return 0;
}

