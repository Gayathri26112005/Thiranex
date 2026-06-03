/*
 * ============================================================
 *         STUDENT MANAGEMENT SYSTEM
 *   Console-based C++ app with file-based persistent storage
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <limits>

using namespace std;

// ─────────────────────────────────────────
//  Data Model
// ─────────────────────────────────────────
struct Student {
    int    id;
    string name;
    string course;
    int    age;
    double gpa;
};

// ─────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────
const string DATA_FILE = "students.csv";
const string DIVIDER   = "─────────────────────────────────────────────────────────────────";

// ─────────────────────────────────────────
//  Utility helpers
// ─────────────────────────────────────────
void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void pause() {
    cout << "\n  Press ENTER to continue...";
    clearInput();
}

void printHeader(const string& title) {
    cout << "\n" << DIVIDER << "\n";
    cout << "  " << title << "\n";
    cout << DIVIDER << "\n";
}

// ─────────────────────────────────────────
//  File I/O  (CSV: id,name,course,age,gpa)
// ─────────────────────────────────────────
vector<Student> loadAll() {
    vector<Student> list;
    ifstream fin(DATA_FILE);
    if (!fin.is_open()) return list;          // first run — no file yet

    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string tok;
        Student s;
        try {
            getline(ss, tok, ','); s.id     = stoi(tok);
            getline(ss, tok, ','); s.name   = tok;
            getline(ss, tok, ','); s.course = tok;
            getline(ss, tok, ','); s.age    = stoi(tok);
            getline(ss, tok, ','); s.gpa    = stod(tok);
            list.push_back(s);
        } catch (...) { /* skip malformed lines */ }
    }
    fin.close();
    return list;
}

void saveAll(const vector<Student>& list) {
    ofstream fout(DATA_FILE, ios::trunc);
    for (const auto& s : list) {
        fout << s.id     << ","
             << s.name   << ","
             << s.course << ","
             << s.age    << ","
             << fixed << setprecision(2) << s.gpa << "\n";
    }
    fout.close();
}

// ─────────────────────────────────────────
//  Auto-increment ID
// ─────────────────────────────────────────
int nextId(const vector<Student>& list) {
    int mx = 0;
    for (const auto& s : list) mx = max(mx, s.id);
    return mx + 1;
}

// ─────────────────────────────────────────
//  Display helpers
// ─────────────────────────────────────────
void printTableHeader() {
    cout << "\n  " << left
         << setw(6)  << "ID"
         << setw(22) << "Name"
         << setw(20) << "Course"
         << setw(6)  << "Age"
         << setw(8)  << "GPA"
         << "\n  " << string(62, '-') << "\n";
}

void printRow(const Student& s) {
    cout << "  " << left
         << setw(6)  << s.id
         << setw(22) << s.name
         << setw(20) << s.course
         << setw(6)  << s.age
         << setw(8)  << fixed << setprecision(2) << s.gpa
         << "\n";
}

// ─────────────────────────────────────────
//  CRUD Operations
// ─────────────────────────────────────────

// 1. ADD
void addStudent() {
    printHeader("ADD NEW STUDENT");
    vector<Student> list = loadAll();
    Student s;
    s.id = nextId(list);

    cout << "  Name    : "; getline(cin, s.name);
    if (s.name.empty()) { cout << "  Name cannot be empty.\n"; return; }

    cout << "  Course  : "; getline(cin, s.course);

    cout << "  Age     : ";
    while (!(cin >> s.age) || s.age <= 0 || s.age > 120) {
        cout << "  Invalid age. Enter again: ";
        clearInput();
    }
    clearInput();

    cout << "  GPA     : ";
    while (!(cin >> s.gpa) || s.gpa < 0.0 || s.gpa > 4.0) {
        cout << "  GPA must be 0.0 – 4.0. Enter again: ";
        clearInput();
    }
    clearInput();

    list.push_back(s);
    saveAll(list);
    cout << "\n  ✔  Student added successfully! (ID = " << s.id << ")\n";
}

// 2. DISPLAY ALL
void displayAll() {
    printHeader("ALL STUDENT RECORDS");
    vector<Student> list = loadAll();
    if (list.empty()) { cout << "  No records found.\n"; return; }

    printTableHeader();
    for (const auto& s : list) printRow(s);
    cout << "\n  Total records: " << list.size() << "\n";
}

// 3. SEARCH
void searchStudent() {
    printHeader("SEARCH STUDENT");
    string key;
    cout << "  Enter name or ID to search: ";
    getline(cin, key);

    vector<Student> list = loadAll();
    bool found = false;
    printTableHeader();
    for (const auto& s : list) {
        // case-insensitive name match or exact ID match
        string lname = s.name, lkey = key;
        transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
        transform(lkey.begin(),  lkey.end(),  lkey.begin(),  ::tolower);
        if (lname.find(lkey) != string::npos || to_string(s.id) == key) {
            printRow(s);
            found = true;
        }
    }
    if (!found) cout << "  No matching student found.\n";
}

// 4. UPDATE
void updateStudent() {
    printHeader("UPDATE STUDENT RECORD");
    int id;
    cout << "  Enter Student ID to update: ";
    while (!(cin >> id)) { cout << "  Invalid ID: "; clearInput(); }
    clearInput();

    vector<Student> list = loadAll();
    auto it = find_if(list.begin(), list.end(), [&](const Student& s){ return s.id == id; });

    if (it == list.end()) { cout << "  Student with ID " << id << " not found.\n"; return; }

    cout << "\n  Current record:\n";
    printTableHeader();
    printRow(*it);
    cout << "\n  Enter new values (press ENTER to keep current):\n";

    // Name
    cout << "  Name [" << it->name << "]: ";
    string tmp; getline(cin, tmp);
    if (!tmp.empty()) it->name = tmp;

    // Course
    cout << "  Course [" << it->course << "]: ";
    getline(cin, tmp);
    if (!tmp.empty()) it->course = tmp;

    // Age
    cout << "  Age [" << it->age << "]: ";
    getline(cin, tmp);
    if (!tmp.empty()) {
        try { int a = stoi(tmp); if (a > 0 && a <= 120) it->age = a; }
        catch (...) {}
    }

    // GPA
    cout << "  GPA [" << fixed << setprecision(2) << it->gpa << "]: ";
    getline(cin, tmp);
    if (!tmp.empty()) {
        try { double g = stod(tmp); if (g >= 0.0 && g <= 4.0) it->gpa = g; }
        catch (...) {}
    }

    saveAll(list);
    cout << "\n  ✔  Record updated successfully!\n";
}

// 5. DELETE
void deleteStudent() {
    printHeader("DELETE STUDENT RECORD");
    int id;
    cout << "  Enter Student ID to delete: ";
    while (!(cin >> id)) { cout << "  Invalid ID: "; clearInput(); }
    clearInput();

    vector<Student> list = loadAll();
    auto it = find_if(list.begin(), list.end(), [&](const Student& s){ return s.id == id; });

    if (it == list.end()) { cout << "  Student with ID " << id << " not found.\n"; return; }

    cout << "\n  Record to delete:\n";
    printTableHeader();
    printRow(*it);
    cout << "\n  Confirm deletion? (y/n): ";
    char ch; cin >> ch; clearInput();

    if (ch == 'y' || ch == 'Y') {
        list.erase(it);
        saveAll(list);
        cout << "\n  ✔  Student deleted successfully!\n";
    } else {
        cout << "  Deletion cancelled.\n";
    }
}

// 6. STATISTICS
void showStats() {
    printHeader("STATISTICS");
    vector<Student> list = loadAll();
    if (list.empty()) { cout << "  No records to analyse.\n"; return; }

    double total = 0, hi = list[0].gpa, lo = list[0].gpa;
    Student topStu = list[0];
    for (const auto& s : list) {
        total += s.gpa;
        if (s.gpa > hi) { hi = s.gpa; topStu = s; }
        if (s.gpa < lo)   lo = s.gpa;
    }
    double avg = total / list.size();

    cout << "  Total students : " << list.size()            << "\n";
    cout << "  Average GPA    : " << fixed << setprecision(2) << avg << "\n";
    cout << "  Highest GPA    : " << hi << "  (" << topStu.name << ")\n";
    cout << "  Lowest GPA     : " << lo  << "\n";
}

// ─────────────────────────────────────────
//  Main menu
// ─────────────────────────────────────────
void showMenu() {
    cout << "\n" << DIVIDER << "\n";
    cout << "       STUDENT MANAGEMENT SYSTEM\n";
    cout << DIVIDER << "\n";
    cout << "  [1]  Add Student\n";
    cout << "  [2]  Display All Students\n";
    cout << "  [3]  Search Student\n";
    cout << "  [4]  Update Student\n";
    cout << "  [5]  Delete Student\n";
    cout << "  [6]  Statistics\n";
    cout << "  [0]  Exit\n";
    cout << DIVIDER << "\n";
    cout << "  Choice: ";
}

int main() {
    int choice;
    do {
        showMenu();
        if (!(cin >> choice)) { clearInput(); choice = -1; }
        else clearInput();

        switch (choice) {
            case 1: addStudent();    break;
            case 2: displayAll();    break;
            case 3: searchStudent(); break;
            case 4: updateStudent(); break;
            case 5: deleteStudent(); break;
            case 6: showStats();     break;
            case 0: cout << "\n  Goodbye!\n\n"; break;
            default: cout << "\n  Invalid option. Please try again.\n";
        }

        if (choice != 0) pause();

    } while (choice != 0);

    return 0;
}
