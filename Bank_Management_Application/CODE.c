/*
 * ================================================================
 *            BANK MANAGEMENT APPLICATION
 *   C++ · Object-Oriented Design · File-Based Persistent Storage
 * ================================================================
 *
 *  Classes
 *  ───────
 *  Transaction   – immutable record of a single debit/credit
 *  Account       – savings / current account with full history
 *  Customer      – owns one or more accounts + personal details
 *  Bank          – top-level manager: file I/O, menus, auth
 *
 *  Storage  (plain-text, one file per entity type)
 *  ───────
 *  customers.dat   –  customer master records
 *  accounts.dat    –  account records
 *  transactions.dat–  ledger entries
 *
 *  Security
 *  ────────
 *  Passwords stored as simple djb2 hashes (no plain-text on disk).
 *  PIN required for every sensitive operation.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <limits>
#include <functional>
#include <climits>

using namespace std;

// ═══════════════════════════════════════════════════════════════
//  Utilities
// ═══════════════════════════════════════════════════════════════
const string LINE  = "═══════════════════════════════════════════════════════════════";
const string DASH  = "───────────────────────────────────────────────────────────────";

void clearIn() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void pause() {
    cout << "\n  Press ENTER to continue...";
    clearIn();
}

void banner(const string& title) {
    cout << "\n" << LINE << "\n";
    cout << "   " << title << "\n";
    cout << LINE << "\n";
}

// Current date-time string  "YYYY-MM-DD HH:MM:SS"
string now() {
    time_t t = time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return buf;
}

// djb2 hash → stored as hex string (no plain-text passwords on disk)
string hashPin(const string& pin) {
    unsigned long h = 5381;
    for (char c : pin) h = ((h << 5) + h) + (unsigned char)c;
    ostringstream oss;
    oss << hex << h;
    return oss.str();
}

// Safe double input
double readDouble(const string& prompt, double lo = 0.0, double hi = 1e12) {
    double v;
    while (true) {
        cout << prompt;
        if (cin >> v && v >= lo && v <= hi) { clearIn(); return v; }
        cout << "  ✘  Invalid amount. Try again.\n";
        clearIn();
    }
}

// Safe int input
int readInt(const string& prompt, int lo = 0, int hi = INT_MAX) {
    int v;
    while (true) {
        cout << prompt;
        if (cin >> v && v >= lo && v <= hi) { clearIn(); return v; }
        cout << "  ✘  Invalid input. Try again.\n";
        clearIn();
    }
}

// ═══════════════════════════════════════════════════════════════
//  Transaction
// ═══════════════════════════════════════════════════════════════
enum class TxType { DEPOSIT, WITHDRAWAL, TRANSFER_OUT, TRANSFER_IN, OPEN };

struct Transaction {
    int         id;
    int         accountId;
    TxType      type;
    double      amount;
    double      balanceAfter;
    string      note;
    string      timestamp;

    string typeStr() const {
        switch (type) {
            case TxType::DEPOSIT:      return "DEPOSIT";
            case TxType::WITHDRAWAL:   return "WITHDRAWAL";
            case TxType::TRANSFER_OUT: return "TRANSFER OUT";
            case TxType::TRANSFER_IN:  return "TRANSFER IN";
            case TxType::OPEN:         return "ACCOUNT OPEN";
            default:                   return "UNKNOWN";
        }
    }

    // Serialise  (fields separated by '|')
    string serialise() const {
        return to_string(id)        + "|" +
               to_string(accountId) + "|" +
               to_string((int)type) + "|" +
               to_string(amount)    + "|" +
               to_string(balanceAfter) + "|" +
               note + "|" +
               timestamp;
    }

    static Transaction deserialise(const string& line) {
        istringstream ss(line);
        string tok;
        Transaction tx;
        getline(ss, tok, '|'); tx.id           = stoi(tok);
        getline(ss, tok, '|'); tx.accountId    = stoi(tok);
        getline(ss, tok, '|'); tx.type         = (TxType)stoi(tok);
        getline(ss, tok, '|'); tx.amount       = stod(tok);
        getline(ss, tok, '|'); tx.balanceAfter = stod(tok);
        getline(ss, tok, '|'); tx.note         = tok;
        getline(ss, tok, '|'); tx.timestamp    = tok;
        return tx;
    }
};

// ═══════════════════════════════════════════════════════════════
//  Account
// ═══════════════════════════════════════════════════════════════
enum class AccType { SAVINGS, CURRENT };

class Account {
public:
    int     id;
    int     customerId;
    AccType type;
    double  balance;
    bool    active;
    string  openedOn;

    Account() : id(0), customerId(0), type(AccType::SAVINGS),
                balance(0), active(true), openedOn(now()) {}

    Account(int id_, int cid, AccType t, double initialDeposit)
        : id(id_), customerId(cid), type(t),
          balance(initialDeposit), active(true), openedOn(now()) {}

    string typeStr() const { return type == AccType::SAVINGS ? "Savings" : "Current"; }

    string serialise() const {
        return to_string(id)         + "|" +
               to_string(customerId) + "|" +
               to_string((int)type)  + "|" +
               to_string(balance)    + "|" +
               (active ? "1" : "0") + "|" +
               openedOn;
    }

    static Account deserialise(const string& line) {
        istringstream ss(line);
        string tok;
        Account a;
        getline(ss, tok, '|'); a.id         = stoi(tok);
        getline(ss, tok, '|'); a.customerId = stoi(tok);
        getline(ss, tok, '|'); a.type       = (AccType)stoi(tok);
        getline(ss, tok, '|'); a.balance    = stod(tok);
        getline(ss, tok, '|'); a.active     = tok == "1";
        getline(ss, tok, '|'); a.openedOn   = tok;
        return a;
    }
};

// ═══════════════════════════════════════════════════════════════
//  Customer
// ═══════════════════════════════════════════════════════════════
class Customer {
public:
    int    id;
    string name;
    string phone;
    string email;
    string pinHash;   // hashed 4-digit PIN
    bool   active;
    string joinedOn;

    Customer() : id(0), active(true), joinedOn(now()) {}

    bool verifyPin(const string& pin) const {
        return hashPin(pin) == pinHash;
    }

    string serialise() const {
        return to_string(id) + "|" +
               name          + "|" +
               phone         + "|" +
               email         + "|" +
               pinHash       + "|" +
               (active ? "1" : "0") + "|" +
               joinedOn;
    }

    static Customer deserialise(const string& line) {
        istringstream ss(line);
        string tok;
        Customer c;
        getline(ss, tok, '|'); c.id       = stoi(tok);
        getline(ss, tok, '|'); c.name     = tok;
        getline(ss, tok, '|'); c.phone    = tok;
        getline(ss, tok, '|'); c.email    = tok;
        getline(ss, tok, '|'); c.pinHash  = tok;
        getline(ss, tok, '|'); c.active   = tok == "1";
        getline(ss, tok, '|'); c.joinedOn = tok;
        return c;
    }
};

// ═══════════════════════════════════════════════════════════════
//  Bank  (top-level controller)
// ═══════════════════════════════════════════════════════════════
class Bank {
    // ── files ──────────────────────────────────────────────────
    const string F_CUSTOMERS    = "customers.dat";
    const string F_ACCOUNTS     = "accounts.dat";
    const string F_TRANSACTIONS = "transactions.dat";

    // ── in-memory collections ──────────────────────────────────
    vector<Customer>    customers;
    vector<Account>     accounts;
    vector<Transaction> transactions;

    // ── session state ──────────────────────────────────────────
    Customer* loggedIn = nullptr;   // currently authenticated customer

    // ════════════════════════════════════════════════════════════
    //  Persistence helpers
    // ════════════════════════════════════════════════════════════
    template<typename T>
    vector<T> loadFile(const string& path,
                       function<T(const string&)> parse) {
        vector<T> v;
        ifstream f(path);
        if (!f.is_open()) return v;
        string line;
        while (getline(f, line))
            if (!line.empty()) {
                try { v.push_back(parse(line)); } catch (...) {}
            }
        return v;
    }

    template<typename T>
    void saveFile(const string& path, const vector<T>& v) {
        ofstream f(path, ios::trunc);
        for (const auto& item : v)
            f << item.serialise() << "\n";
    }

    void load() {
        customers    = loadFile<Customer>   (F_CUSTOMERS,    Customer::deserialise);
        accounts     = loadFile<Account>    (F_ACCOUNTS,     Account::deserialise);
        transactions = loadFile<Transaction>(F_TRANSACTIONS, Transaction::deserialise);
    }

    void save() {
        saveFile(F_CUSTOMERS,    customers);
        saveFile(F_ACCOUNTS,     accounts);
        saveFile(F_TRANSACTIONS, transactions);
    }

    // ════════════════════════════════════════════════════════════
    //  ID generators
    // ════════════════════════════════════════════════════════════
    int nextCustId() {
        int mx = 0;
        for (auto& c : customers) mx = max(mx, c.id);
        return mx + 1;
    }
    int nextAccId() {
        int mx = 0;
        for (auto& a : accounts) mx = max(mx, a.id);
        return mx + 1;
    }
    int nextTxId() {
        int mx = 0;
        for (auto& t : transactions) mx = max(mx, t.id);
        return mx + 1;
    }

    // ════════════════════════════════════════════════════════════
    //  Lookup helpers
    // ════════════════════════════════════════════════════════════
    Customer* findCustomer(int id) {
        for (auto& c : customers)
            if (c.id == id && c.active) return &c;
        return nullptr;
    }
    Account* findAccount(int id) {
        for (auto& a : accounts)
            if (a.id == id && a.active) return &a;
        return nullptr;
    }

    vector<Account*> myAccounts() {
        vector<Account*> v;
        for (auto& a : accounts)
            if (a.customerId == loggedIn->id && a.active)
                v.push_back(&a);
        return v;
    }

    // ════════════════════════════════════════════════════════════
    //  Transaction recorder
    // ════════════════════════════════════════════════════════════
    void record(int accId, TxType type, double amount,
                double balAfter, const string& note) {
        Transaction tx;
        tx.id           = nextTxId();
        tx.accountId    = accId;
        tx.type         = type;
        tx.amount       = amount;
        tx.balanceAfter = balAfter;
        tx.note         = note;
        tx.timestamp    = now();
        transactions.push_back(tx);
    }

    // ════════════════════════════════════════════════════════════
    //  PIN prompt
    // ════════════════════════════════════════════════════════════
    bool confirmPin(const string& prompt = "  Enter PIN to confirm: ") {
        cout << prompt;
        string pin; getline(cin, pin);
        if (!loggedIn->verifyPin(pin)) {
            cout << "  ✘  Incorrect PIN. Operation cancelled.\n";
            return false;
        }
        return true;
    }

    // ════════════════════════════════════════════════════════════
    //  Display helpers
    // ════════════════════════════════════════════════════════════
    void printAccountRow(const Account& a) {
        cout << "  " << left
             << setw(8)  << a.id
             << setw(12) << a.typeStr()
             << "  ₹ " << right << setw(14)
             << fixed << setprecision(2) << a.balance
             << "    " << a.openedOn << "\n";
    }

    void printTxRow(const Transaction& tx) {
        string sign = (tx.type == TxType::DEPOSIT ||
                       tx.type == TxType::TRANSFER_IN ||
                       tx.type == TxType::OPEN) ? "+" : "-";
        cout << "  " << left
             << setw(6)  << tx.id
             << setw(20) << tx.timestamp
             << setw(16) << tx.typeStr()
             << sign << " ₹ " << right << setw(12)
             << fixed << setprecision(2) << tx.amount
             << "   Bal: ₹ " << setw(12) << tx.balanceAfter
             << "\n";
    }

    // ════════════════════════════════════════════════════════════
    //  ── CUSTOMER OPERATIONS ──
    // ════════════════════════════════════════════════════════════

    // Register new customer
    void registerCustomer() {
        banner("NEW CUSTOMER REGISTRATION");
        Customer c;
        c.id = nextCustId();

        cout << "  Full Name  : "; getline(cin, c.name);
        if (c.name.empty()) { cout << "  Name cannot be empty.\n"; return; }

        cout << "  Phone      : "; getline(cin, c.phone);
        cout << "  Email      : "; getline(cin, c.email);

        string pin, pin2;
        while (true) {
            cout << "  Set PIN (4 digits): "; getline(cin, pin);
            if (pin.size() != 4 || !all_of(pin.begin(), pin.end(), ::isdigit)) {
                cout << "  ✘  PIN must be exactly 4 digits.\n"; continue;
            }
            cout << "  Confirm PIN       : "; getline(cin, pin2);
            if (pin != pin2) { cout << "  ✘  PINs do not match.\n"; continue; }
            break;
        }
        c.pinHash  = hashPin(pin);
        c.active   = true;
        c.joinedOn = now();

        customers.push_back(c);
        save();
        cout << "\n  ✔  Registration successful!  Customer ID = " << c.id << "\n";
        cout << "  Please note your Customer ID – needed for login.\n";
    }

    // Login
    bool login() {
        banner("CUSTOMER LOGIN");
        int id = readInt("  Customer ID : ", 1);
        Customer* c = findCustomer(id);
        if (!c) { cout << "  ✘  Customer not found or account closed.\n"; return false; }

        cout << "  PIN         : ";
        string pin; getline(cin, pin);
        if (!c->verifyPin(pin)) {
            cout << "  ✘  Incorrect PIN.\n"; return false;
        }
        loggedIn = c;
        cout << "\n  ✔  Welcome, " << loggedIn->name << "!\n";
        return true;
    }

    // ════════════════════════════════════════════════════════════
    //  ── ACCOUNT OPERATIONS ──
    // ════════════════════════════════════════════════════════════

    void openAccount() {
        banner("OPEN NEW ACCOUNT");
        cout << "  Account type:\n  [1] Savings\n  [2] Current\n";
        int ch = readInt("  Choice: ", 1, 2);
        AccType t = (ch == 1) ? AccType::SAVINGS : AccType::CURRENT;

        double minDep = (t == AccType::SAVINGS) ? 500.0 : 1000.0;
        cout << "  Minimum opening deposit: ₹ " << fixed << setprecision(2) << minDep << "\n";
        double dep = readDouble("  Opening deposit (₹): ", minDep);

        Account a(nextAccId(), loggedIn->id, t, dep);
        accounts.push_back(a);
        record(a.id, TxType::OPEN, dep, dep, "Account opened");
        save();
        cout << "\n  ✔  Account opened!  Account No. = " << a.id << "\n";
    }

    void deposit() {
        banner("DEPOSIT");
        auto accs = myAccounts();
        if (accs.empty()) { cout << "  No active accounts found.\n"; return; }

        cout << "  Account No  Account Type    Balance\n  " << DASH << "\n";
        for (auto* a : accs) printAccountRow(*a);

        int aid = readInt("\n  Account No to deposit into: ", 1);
        Account* acc = findAccount(aid);
        if (!acc || acc->customerId != loggedIn->id) {
            cout << "  ✘  Invalid account.\n"; return;
        }

        double amt = readDouble("  Amount (₹): ", 1.0);
        if (!confirmPin()) return;

        acc->balance += amt;
        record(acc->id, TxType::DEPOSIT, amt, acc->balance, "Cash deposit");
        save();
        cout << "\n  ✔  Deposited ₹ " << fixed << setprecision(2) << amt
             << "  |  New Balance: ₹ " << acc->balance << "\n";
    }

    void withdraw() {
        banner("WITHDRAWAL");
        auto accs = myAccounts();
        if (accs.empty()) { cout << "  No active accounts found.\n"; return; }

        cout << "  Account No  Account Type    Balance\n  " << DASH << "\n";
        for (auto* a : accs) printAccountRow(*a);

        int aid = readInt("\n  Account No to withdraw from: ", 1);
        Account* acc = findAccount(aid);
        if (!acc || acc->customerId != loggedIn->id) {
            cout << "  ✘  Invalid account.\n"; return;
        }

        double minBal = (acc->type == AccType::SAVINGS) ? 500.0 : 1000.0;
        double maxWith = acc->balance - minBal;
        if (maxWith <= 0) {
            cout << "  ✘  Insufficient balance (minimum balance ₹ "
                 << fixed << setprecision(2) << minBal << " must be maintained).\n";
            return;
        }

        cout << "  Available for withdrawal: ₹ " << fixed << setprecision(2) << maxWith << "\n";
        double amt = readDouble("  Amount (₹): ", 1.0, maxWith);
        if (!confirmPin()) return;

        acc->balance -= amt;
        record(acc->id, TxType::WITHDRAWAL, amt, acc->balance, "Cash withdrawal");
        save();
        cout << "\n  ✔  Withdrawn ₹ " << fixed << setprecision(2) << amt
             << "  |  New Balance: ₹ " << acc->balance << "\n";
    }

    void checkBalance() {
        banner("BALANCE ENQUIRY");
        auto accs = myAccounts();
        if (accs.empty()) { cout << "  No active accounts found.\n"; return; }

        double total = 0;
        cout << "\n  " << left
             << setw(8)  << "Acc No."
             << setw(12) << "Type"
             << "  Balance\n  " << DASH << "\n";
        for (auto* a : accs) {
            printAccountRow(*a);
            total += a->balance;
        }
        cout << "  " << DASH << "\n";
        cout << "  Total across all accounts: ₹ "
             << fixed << setprecision(2) << total << "\n";
    }

    void transfer() {
        banner("FUND TRANSFER");
        auto accs = myAccounts();
        if (accs.empty()) { cout << "  No active accounts found.\n"; return; }

        cout << "  Your accounts:\n";
        for (auto* a : accs) printAccountRow(*a);

        int fromId = readInt("\n  From Account No: ", 1);
        Account* from = findAccount(fromId);
        if (!from || from->customerId != loggedIn->id) {
            cout << "  ✘  Invalid source account.\n"; return;
        }

        int toId = readInt("  To Account No  : ", 1);
        if (toId == fromId) { cout << "  ✘  Cannot transfer to the same account.\n"; return; }
        Account* to = findAccount(toId);
        if (!to) { cout << "  ✘  Destination account not found.\n"; return; }

        double minBal   = (from->type == AccType::SAVINGS) ? 500.0 : 1000.0;
        double maxTrans = from->balance - minBal;
        if (maxTrans <= 0) {
            cout << "  ✘  Insufficient balance for transfer.\n"; return;
        }

        cout << "  Available: ₹ " << fixed << setprecision(2) << maxTrans << "\n";
        double amt = readDouble("  Amount (₹): ", 1.0, maxTrans);
        if (!confirmPin()) return;

        from->balance -= amt;
        to->balance   += amt;

        record(from->id, TxType::TRANSFER_OUT, amt, from->balance,
               "Transfer to Acc#" + to_string(toId));
        record(to->id,   TxType::TRANSFER_IN,  amt, to->balance,
               "Transfer from Acc#" + to_string(fromId));
        save();

        cout << "\n  ✔  Transferred ₹ " << fixed << setprecision(2) << amt
             << "  to Account #" << toId << "\n";
        cout << "     Your balance: ₹ " << from->balance << "\n";
    }

    void miniStatement() {
        banner("MINI STATEMENT  (last 10 transactions)");
        auto accs = myAccounts();
        if (accs.empty()) { cout << "  No active accounts found.\n"; return; }

        cout << "  Your accounts:\n";
        for (auto* a : accs) printAccountRow(*a);

        int aid = readInt("\n  Account No: ", 1);
        Account* acc = findAccount(aid);
        if (!acc || acc->customerId != loggedIn->id) {
            cout << "  ✘  Invalid account.\n"; return;
        }

        vector<Transaction*> mine;
        for (auto& tx : transactions)
            if (tx.accountId == aid) mine.push_back(&tx);

        if (mine.empty()) { cout << "  No transactions found.\n"; return; }

        // Show last 10
        int start = max(0, (int)mine.size() - 10);
        cout << "\n  " << left
             << setw(6)  << "ID"
             << setw(20) << "Date/Time"
             << setw(16) << "Type"
             << setw(18) << "Amount"
             << "Balance\n  " << DASH << "\n";
        for (int i = start; i < (int)mine.size(); ++i)
            printTxRow(*mine[i]);
    }

    void fullStatement() {
        banner("FULL ACCOUNT STATEMENT");
        auto accs = myAccounts();
        if (accs.empty()) { cout << "  No active accounts found.\n"; return; }

        for (auto* a : accs) printAccountRow(*a);

        int aid = readInt("\n  Account No: ", 1);
        Account* acc = findAccount(aid);
        if (!acc || acc->customerId != loggedIn->id) {
            cout << "  ✘  Invalid account.\n"; return;
        }

        if (!confirmPin()) return;

        cout << "\n  " << left
             << setw(6)  << "ID"
             << setw(20) << "Date/Time"
             << setw(16) << "Type"
             << setw(18) << "Amount"
             << "Balance\n  " << DASH << "\n";
        bool any = false;
        for (auto& tx : transactions)
            if (tx.accountId == aid) { printTxRow(tx); any = true; }
        if (!any) cout << "  No transactions on record.\n";
    }

    void updateProfile() {
        banner("UPDATE PROFILE");
        cout << "  Current details:\n";
        cout << "  Name  : " << loggedIn->name  << "\n";
        cout << "  Phone : " << loggedIn->phone << "\n";
        cout << "  Email : " << loggedIn->email << "\n\n";
        cout << "  (Press ENTER to keep current value)\n\n";

        string tmp;
        cout << "  New Name  [" << loggedIn->name  << "]: "; getline(cin, tmp);
        if (!tmp.empty()) loggedIn->name  = tmp;

        cout << "  New Phone [" << loggedIn->phone << "]: "; getline(cin, tmp);
        if (!tmp.empty()) loggedIn->phone = tmp;

        cout << "  New Email [" << loggedIn->email << "]: "; getline(cin, tmp);
        if (!tmp.empty()) loggedIn->email = tmp;

        if (!confirmPin()) return;
        save();
        cout << "\n  ✔  Profile updated.\n";
    }

    void changePin() {
        banner("CHANGE PIN");
        cout << "  Current PIN : "; string cur; getline(cin, cur);
        if (!loggedIn->verifyPin(cur)) {
            cout << "  ✘  Incorrect current PIN.\n"; return;
        }

        string np, np2;
        while (true) {
            cout << "  New PIN (4 digits): "; getline(cin, np);
            if (np.size() != 4 || !all_of(np.begin(), np.end(), ::isdigit)) {
                cout << "  ✘  PIN must be exactly 4 digits.\n"; continue;
            }
            cout << "  Confirm new PIN   : "; getline(cin, np2);
            if (np != np2) { cout << "  ✘  PINs do not match.\n"; continue; }
            break;
        }
        loggedIn->pinHash = hashPin(np);
        save();
        cout << "\n  ✔  PIN changed successfully.\n";
    }

    void closeAccount() {
        banner("CLOSE ACCOUNT");
        auto accs = myAccounts();
        if (accs.empty()) { cout << "  No active accounts to close.\n"; return; }

        for (auto* a : accs) printAccountRow(*a);
        int aid = readInt("\n  Account No to close: ", 1);
        Account* acc = findAccount(aid);
        if (!acc || acc->customerId != loggedIn->id) {
            cout << "  ✘  Invalid account.\n"; return;
        }
        if (acc->balance > 0) {
            cout << "  Remaining balance ₹ " << fixed << setprecision(2)
                 << acc->balance << " will be paid out. Proceed? (y/n): ";
            char ch; cin >> ch; clearIn();
            if (ch != 'y' && ch != 'Y') { cout << "  Cancelled.\n"; return; }
        }
        if (!confirmPin()) return;

        acc->active = false;
        record(acc->id, TxType::WITHDRAWAL, acc->balance, 0.0,
               "Account closed – final payout");
        acc->balance = 0;
        save();
        cout << "\n  ✔  Account #" << aid << " closed.\n";
    }

    // ════════════════════════════════════════════════════════════
    //  ── MENUS ──
    // ════════════════════════════════════════════════════════════
    void accountMenu() {
        while (true) {
            banner("ACCOUNT OPERATIONS  —  " + loggedIn->name);
            cout << "  [1]  Open New Account\n";
            cout << "  [2]  Deposit\n";
            cout << "  [3]  Withdraw\n";
            cout << "  [4]  Fund Transfer\n";
            cout << "  [5]  Balance Enquiry\n";
            cout << "  [6]  Mini Statement (last 10)\n";
            cout << "  [7]  Full Statement\n";
            cout << "  [8]  Update Profile\n";
            cout << "  [9]  Change PIN\n";
            cout << "  [10] Close Account\n";
            cout << "  [0]  Logout\n";
            cout << LINE << "\n  Choice: ";

            int ch = readInt("", 0, 10);
            switch (ch) {
                case 1:  openAccount();   break;
                case 2:  deposit();       break;
                case 3:  withdraw();      break;
                case 4:  transfer();      break;
                case 5:  checkBalance();  break;
                case 6:  miniStatement(); break;
                case 7:  fullStatement(); break;
                case 8:  updateProfile(); break;
                case 9:  changePin();     break;
                case 10: closeAccount();  break;
                case 0:
                    cout << "\n  Logged out. Goodbye, "
                         << loggedIn->name << "!\n";
                    loggedIn = nullptr;
                    return;
            }
            if (ch != 0) pause();
        }
    }

public:
    // ════════════════════════════════════════════════════════════
    //  Entry point
    // ════════════════════════════════════════════════════════════
    void run() {
        load();

        while (true) {
            banner("NEXUS BANK  —  MAIN MENU");
            cout << "  [1]  Login\n";
            cout << "  [2]  Register as New Customer\n";
            cout << "  [0]  Exit\n";
            cout << LINE << "\n  Choice: ";

            int ch = readInt("", 0, 2);
            switch (ch) {
                case 1:
                    if (login()) accountMenu();
                    else pause();
                    break;
                case 2:
                    registerCustomer();
                    pause();
                    break;
                case 0:
                    cout << "\n  Thank you for banking with NEXUS BANK. Goodbye!\n\n";
                    return;
            }
        }
    }
};

// ════════════════════════════════════════════════════════════════
//  main
// ════════════════════════════════════════════════════════════════
int main() {
    Bank bank;
    bank.run();
    return 0;
}
