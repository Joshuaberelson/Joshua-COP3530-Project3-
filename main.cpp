#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cctype>
#include <chrono>
#include <iomanip>

using namespace std;

// Struct to hold name data
struct NameRecord {
    string name;
    char sex;
    int year;
    int count;
};

// Node for Trie data structure
struct TrieNode {
    TrieNode* children[26]; //a-z
    bool isEndOfWord;
    int popularityCount;
    string fullName;

    TrieNode() {
        for (int i = 0; i < 26; i++)
            children[i] = nullptr;
        isEndOfWord = false;
        popularityCount = 0;
    }
};

//Trie class for name searching
class Trie {
private:
    TrieNode* root;

public:
    Trie() {
        root = new TrieNode();
    }

    //Insert a name with its popularity count into the trie
    void insert(string name, int count) {
        TrieNode* current = root;

        //Convert to lowercase for search
        string lowerName = name;
        for (char& c : lowerName)
            c = tolower(c);

        for (char c : lowerName) {
            int index = c - 'a';
            if (index < 0 || index >= 26) continue; //skip non-letters

            if (current->children[index] == nullptr)
                current->children[index] = new TrieNode();
            current = current->children[index];
        }

        current->isEndOfWord = true;
        current->fullName = name;
        current->popularityCount += count; // add to existing count
    }

    //Search for exact name and return popularity
    int search(string name) {
        //Convert to lowercase
        string lowerName = name;
        for (char& c : lowerName)
            c = tolower(c);
        TrieNode* current = root;

        //Navigate through the trie
        for (char c : lowerName) {
            int index = c - 'a';
            if (index < 0 || index >= 26 || current->children[index] == nullptr)
                return 0; //not found
            current = current->children[index];
        }

        if (current->isEndOfWord)
            return current->popularityCount;
        return 0; //not found
    }
};

//Hash Table implementation
class HashTable {
private:
    struct Entry {
        string key;
        int value;
        bool occupied;
        bool deleted;
        Entry() : value(0), occupied(false), deleted(false) {}
    };

    vector<Entry> table;
    int capacity;
    int size;

    //Hash function
    int hashFunction(const string& key) {
        int hash = 0;
        for (char c : key)
            hash = (hash * 31 + c) % capacity;
        return hash;
    }

    //Resize when load factor is too high
    void resize() {
        vector<Entry> oldTable = table;
        capacity *= 2;
        table = vector<Entry>(capacity);
        size = 0;

        for (const Entry& entry : oldTable) {
            if (entry.occupied && !entry.deleted)
                insert(entry.key, entry.value);
        }
    }

public:
    HashTable(int initialCapacity = 10000) {
        capacity = initialCapacity;
        size = 0;
        table = vector<Entry>(capacity);
    }

    //Insert a key-value pair
    void insert(const string& key, int value) {
        if ((double)size / capacity > 0.7)
            resize();

        int index = hashFunction(key);
        int originalIndex = index;

        //Linear probing for resolution
        while (table[index].occupied && !table[index].deleted && table[index].key != key) {
            index = (index + 1) % capacity;
            if (index == originalIndex) {
                resize();
                insert(key, value);
                return;
            }
        }

        if (!table[index].occupied || table[index].deleted)
            size++;

        table[index].key = key;
        table[index].value = value;
        table[index].occupied = true;
        table[index].deleted = false;
    }

    //Get value for a key
    int get(const string& key) {
        int index = hashFunction(key);
        int originalIndex = index;

        while (table[index].occupied) {
            if (!table[index].deleted && table[index].key == key)
                return table[index].value;
            index = (index + 1) % capacity;
            if (index == originalIndex)
                break;
        }

        return 0; //Not found
    }

    //Get all keys
    vector<string> getAllKeys() {
        vector<string> keys;
        for (const Entry& entry : table) {
            if (entry.occupied && !entry.deleted)
                keys.push_back(entry.key);
        }
        return keys;
    }
};

//Main database class
class BabyNameData {
private:
    vector<NameRecord> allRecords;
    HashTable nameTotalCounts; //custom hash table for total counts
    Trie nameTrie;             //trie for name searching

    //Helper to normalize name
    string normalizeName(string name) {
        if (!name.empty()) {
            name[0] = toupper(name[0]);
            for (size_t i = 1; i < name.length(); i++)
                name[i] = tolower(name[i]);
        }
        return name;
    }

public:
    //Load data from CSV file
    bool loadData(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "Error: Could not open file " << filename << endl;
            return false;
        }

        string line;
        int count = 0;
        cout << "Loading data into both Hash Table and Trie..." << endl;

        while (getline(file, line)) {
            stringstream ss(line);
            string name, sexStr, yearStr, countStr;

            //Parse CSV: name,sex,year,count
            if (getline(ss, name, ',') &&
                getline(ss, sexStr, ',') &&
                getline(ss, yearStr, ',') &&
                getline(ss, countStr, ',')) {

                NameRecord record;
                record.name = name;
                record.sex = sexStr[0];
                record.year = stoi(yearStr);
                record.count = stoi(countStr);
                allRecords.push_back(record);

                //Update total counts in hash table
                int currentCount = nameTotalCounts.get(name);
                nameTotalCounts.insert(name, currentCount + record.count);

                //Add to trie with count
                nameTrie.insert(name, record.count);
                count++;
                if (count % 50000 == 0)
                    cout << "  Loaded " << count << " records..." << endl;
            }
        }

        file.close();
        cout << "Successfully loaded " << count << " records!" << endl;
        cout << "Both data structures are ready for comparison.\n" << endl;
        return true;
    }

    //Search name in both structures and compare performance
    void searchNameComparison(string name) {
        name = normalizeName(name);

        cout << "\n========================================" << endl;
        cout << "Searching for: " << name << endl;
        cout << "========================================\n" << endl;

        //Search in Hash Table
        cout << "[1] Hash Table Search:" << endl;
        auto start = chrono::high_resolution_clock::now();
        int hashResult = nameTotalCounts.get(name);
        auto end = chrono::high_resolution_clock::now();
        double hashTime = chrono::duration<double, micro>(end - start).count();

        cout << "    Result: ";
        if (hashResult > 0)
            cout << hashResult << " babies" << endl;
        else
            cout << "Not found" << endl;
        cout << "    Time: " << fixed << setprecision(4) << hashTime << " microseconds\n" << endl;

        //Search in Trie
        cout << "[2] Trie Search:" << endl;
        start = chrono::high_resolution_clock::now();
        int trieResult = nameTrie.search(name);
        end = chrono::high_resolution_clock::now();
        double trieTime = chrono::duration<double, micro>(end - start).count();

        cout << "    Result: ";
        if (trieResult > 0)
            cout << trieResult << " babies" << endl;
        else
            cout << "Not found" << endl;
        cout << "    Time: " << fixed << setprecision(4) << trieTime << " microseconds\n" << endl;

        // erformance comparison
        cout << "--- Performance Comparison ---" << endl;
        if (hashTime < trieTime) {
            double ratio = trieTime / hashTime;
            cout << "Hash Table was " << fixed << setprecision(2) << ratio << "x FASTER" << endl;
        }
        else if (trieTime < hashTime) {
            double ratio = hashTime / trieTime;
            cout << "Trie was " << fixed << setprecision(2) << ratio << "x FASTER" << endl;
        }
        else
            cout << "Both structures had equal performance!" << endl;
        cout << "========================================" << endl;
    }

    //Compare two names using both structures
    void compareTwoNames(string name1, string name2) {
        name1 = normalizeName(name1);
        name2 = normalizeName(name2);

        cout << "\n========================================" << endl;
        cout << "Comparing: " << name1 << " vs " << name2 << endl;
        cout << "========================================\n" << endl;

        //Search name1 in both structures
        cout << "Searching for " << name1 << "..." << endl;

        auto start = chrono::high_resolution_clock::now();
        int hash1 = nameTotalCounts.get(name1);
        auto end = chrono::high_resolution_clock::now();
        double hashTime1 = chrono::duration<double, micro>(end - start).count();

        start = chrono::high_resolution_clock::now();
        int trie1 = nameTrie.search(name1);
        end = chrono::high_resolution_clock::now();
        double trieTime1 = chrono::duration<double, micro>(end - start).count();

        cout << "  Hash Table: " << hash1 << " babies (" << hashTime1 << " microseconds)" << endl;
        cout << "  Trie:       " << trie1 << " babies (" << trieTime1 << " microseconds)\n" << endl;

        //Search name2 in both structures
        cout << "Searching for " << name2 << "..." << endl;

        start = chrono::high_resolution_clock::now();
        int hash2 = nameTotalCounts.get(name2);
        end = chrono::high_resolution_clock::now();
        double hashTime2 = chrono::duration<double, micro>(end - start).count();

        start = chrono::high_resolution_clock::now();
        int trie2 = nameTrie.search(name2);
        end = chrono::high_resolution_clock::now();
        double trieTime2 = chrono::duration<double, micro>(end - start).count();

        cout << "  Hash Table: " << hash2 << " babies (" << hashTime2 << " microseconds)" << endl;
        cout << "  Trie:       " << trie2 << " babies (" << trieTime2 << " microseconds)\n" << endl;

        //Results
        cout << "--- Results ---" << endl;
        if (hash1 > hash2)
            cout << name1 << " is MORE POPULAR (" << hash1 << " vs " << hash2 << ")" << endl;
        else if (hash2 > hash1)
            cout << name2 << " is MORE POPULAR (" << hash2 << " vs " << hash1 << ")" << endl;
        else if (hash1 == 0 && hash2 == 0)
            cout << "Neither name found in database." << endl;
        else
            cout << "Both names are equally popular! (" << hash1 << " each)" << endl;

        //Average performance comparison
        double avgHashTime = (hashTime1 + hashTime2) / 2.0;
        double avgTrieTime = (trieTime1 + trieTime2) / 2.0;

        cout << "\n--- Average Search Performance ---" << endl;
        cout << "Hash Table avg: " << fixed << setprecision(4) << avgHashTime << " microseconds" << endl;
        cout << "Trie avg:       " << fixed << setprecision(4) << avgTrieTime << " microseconds" << endl;

        if (avgHashTime < avgTrieTime) {
            cout << "Hash Table was " << setprecision(2) << (avgTrieTime / avgHashTime)
                 << "x faster on average" << endl;
        }
        else {
            cout << "Trie was " << setprecision(2) << (avgHashTime / avgTrieTime)
                 << "x faster on average" << endl;
        }
        cout << "========================================" << endl;
    }

    //Get top 10 names in a given year
    void getTop10InYear(int year) {
        HashTable yearCounts(1000);

        //Aggregate counts for this year
        for (const NameRecord& record : allRecords) {
            if (record.year == year) {
                int currentCount = yearCounts.get(record.name);
                yearCounts.insert(record.name, currentCount + record.count);
            }
        }

        //Get all names and their counts
        vector<pair<string, int>> nameCounts;
        vector<string> allNames = yearCounts.getAllKeys();

        if (allNames.empty()) {
            cout << "\nNo data available for year " << year << endl;
            return;
        }

        for (const string& name : allNames)
            nameCounts.push_back({name, yearCounts.get(name)});

        //Sort by count (descending)
        sort(nameCounts.begin(), nameCounts.end(),
             [](const pair<string, int>& a, const pair<string, int>& b) {
                 return a.second > b.second;
             });

        //Display top 10
        cout << "\n========================================" << endl;
        cout << "     Top 10 Names in " << year << endl;
        cout << "========================================" << endl;
        int limit = min(10, (int)nameCounts.size());
        for (int i = 0; i < limit; i++) {
            cout << setw(2) << (i + 1) << ". " << left << setw(15) << nameCounts[i].first
                 << right << setw(10) << nameCounts[i].second << " babies" << endl;
        }
        cout << "========================================" << endl;
    }

    //Performance analysis report
    void performanceReport() {
        cout << "\n========================================" << endl;
        cout << "     Performance Analysis Report" << endl;
        cout << "========================================\n" << endl;

        //Test with a few sample names
        vector<string> testNames = {"Emma", "Liam", "Olivia", "Noah", "Ava"};

        double totalHashTime = 0;
        double totalTrieTime = 0;
        int iterations = testNames.size();

        cout << "Testing " << iterations << " sample names...\n" << endl;

        for (const string& name : testNames) {
            //Hash Table
            auto start = chrono::high_resolution_clock::now();
            nameTotalCounts.get(name);
            auto end = chrono::high_resolution_clock::now();
            double hashTime = chrono::duration<double, micro>(end - start).count();
            totalHashTime += hashTime;

            //Trie
            start = chrono::high_resolution_clock::now();
            nameTrie.search(name);
            end = chrono::high_resolution_clock::now();
            double trieTime = chrono::duration<double, micro>(end - start).count();
            totalTrieTime += trieTime;

            cout << name << ":" << endl;
            cout << "  Hash: " << hashTime << " microseconds | Trie: " << trieTime << " microseconds" << endl;
        }

        double avgHash = totalHashTime / iterations;
        double avgTrie = totalTrieTime / iterations;

        cout << "\n--- Summary ---" << endl;
        cout << "Average Hash Table time: " << fixed << setprecision(4) << avgHash << " microseconds" << endl;
        cout << "Average Trie time:       " << fixed << setprecision(4) << avgTrie << " microseconds" << endl;

        cout << "\n--- Analysis ---" << endl;

        if (avgHash < avgTrie) {
            cout << "Hash Table performed " << setprecision(2)
                 << (avgTrie / avgHash) << "x faster on average" << endl;
            cout << "Hash Table is more efficient for exact name lookups." << endl;
        }
        else {
            cout << "Trie performed " << setprecision(2)
                 << (avgHash / avgTrie) << "x faster on average" << endl;
            cout << "Trie is more efficient for exact name lookups." << endl;
        }

        cout << "========================================" << endl;
    }
};

//Main menu system
void displayMenu() {
    cout << "\n—-----------------------------------------------" << endl;
    cout << "          Baby Name Explorer" << endl;
    cout << "     (Hash Table vs Trie Comparison)" << endl;
    cout << "—-----------------------------------------------" << endl;
    cout << "[0] Exit" << endl;
    cout << "[1] Search name popularity (compare structures)" << endl;
    cout << "[2] Top 10 names in a given year" << endl;
    cout << "[3] Compare two names (with performance)" << endl;
    cout << "[4] Performance Analysis Report" << endl;
    cout << "—-----------------------------------------------" << endl;
    cout << "Enter your choice: ";
}

int main() {
    BabyNameData database;

    cout << "========================================" << endl;
    cout << "   Welcome to Baby Name Explorer!" << endl;
    cout << "========================================" << endl;

    //Load the data file
    if (!database.loadData("../resources/babynames.csv")) {
        cout << "Failed to load data. Please ensure babynames.csv is in the same directory." << endl;
        return 1;
    }

    int choice;

    while (true) {
        displayMenu();
        cin >> choice;

        if (choice == 0) {
            cout << "\nThanks for using Baby Name Explorer! Goodbye!" << endl;
            break;
        }

        switch (choice) {
            case 1: {
                string name;
                cout << "\nEnter a name: ";
                cin >> name;
                database.searchNameComparison(name);
                break;
            }
            case 2: {
                int year;
                //keep asking until valid 2000–2024
                while (true) {
                    cout << "\nEnter a year (2000-2024): ";
                    cin >> year;

                    if (year >= 2000 && year <= 2024) {
                        database.getTop10InYear(year);
                        break;
                    } else {
                        cout << "Invalid year. Please try again.\n";
                    }
                }
                break;
            }
            case 3: {
                string name1, name2;
                cout << "\nEnter first name: ";
                cin >> name1;
                cout << "Enter second name: ";
                cin >> name2;
                database.compareTwoNames(name1, name2);
                break;
            }
            case 4: {
                database.performanceReport();
                break;
            }
            default:
                cout << "\nInvalid choice. Please try again." << endl;
        }
    }

    return 0;
}