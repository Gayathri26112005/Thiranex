#include <iostream>
#include <vector>
#include <string>
using namespace std;

class Book {
public:
    int bookId;
    string title;
    string author;
    bool issued;

    Book(int id, string t, string a) {
        bookId = id;
        title = t;
        author = a;
        issued = false;
    }
};

class Library {
private:
    vector<Book> books;

public:
    void addBook() {
        int id;
        string title, author;

        cout << "\nEnter Book ID: ";
        cin >> id;
        cin.ignore();

        cout << "Enter Book Title: ";
        getline(cin, title);

        cout << "Enter Author Name: ";
        getline(cin, author);

        books.push_back(Book(id, title, author));
        cout << "Book added successfully!\n";
    }

    void displayBooks() {
        if (books.empty()) {
            cout << "\nNo books available.\n";
            return;
        }

        cout << "\n----- Book List -----\n";
        for (auto &book : books) {
            cout << "ID: " << book.bookId
                 << "\nTitle: " << book.title
                 << "\nAuthor: " << book.author
                 << "\nStatus: " << (book.issued ? "Issued" : "Available")
                 << "\n-------------------\n";
        }
    }

    void searchByTitle() {
        string searchTitle;
        cin.ignore();

        cout << "\nEnter title to search: ";
        getline(cin, searchTitle);

        bool found = false;

        for (auto &book : books) {
            if (book.title == searchTitle) {
                cout << "\nBook Found!\n";
                cout << "ID: " << book.bookId
                     << "\nAuthor: " << book.author
                     << "\nStatus: "
                     << (book.issued ? "Issued" : "Available") << endl;
                found = true;
            }
        }

        if (!found)
            cout << "Book not found.\n";
    }

    void searchByAuthor() {
        string searchAuthor;
        cin.ignore();

        cout << "\nEnter author name to search: ";
        getline(cin, searchAuthor);

        bool found = false;

        for (auto &book : books) {
            if (book.author == searchAuthor) {
                cout << "\nBook Found!\n";
                cout << "ID: " << book.bookId
                     << "\nTitle: " << book.title
                     << "\nStatus: "
                     << (book.issued ? "Issued" : "Available") << endl;
                found = true;
            }
        }

        if (!found)
            cout << "No books found by this author.\n";
    }

    void issueBook() {
        int id;
        cout << "\nEnter Book ID to issue: ";
        cin >> id;

        for (auto &book : books) {
            if (book.bookId == id) {
                if (!book.issued) {
                    book.issued = true;
                    cout << "Book issued successfully!\n";
                } else {
                    cout << "Book is already issued.\n";
                }
                return;
            }
        }

        cout << "Book not found.\n";
    }

    void returnBook() {
        int id;
        cout << "\nEnter Book ID to return: ";
        cin >> id;

        for (auto &book : books) {
            if (book.bookId == id) {
                if (book.issued) {
                    book.issued = false;
                    cout << "Book returned successfully!\n";
                } else {
                    cout << "Book was not issued.\n";
                }
                return;
            }
        }

        cout << "Book not found.\n";
    }
};

int main() {
    Library lib;
    int choice;

    do {
        cout << "\n====== Library Management System ======\n";
        cout << "1. Add Book\n";
        cout << "2. Display Books\n";
        cout << "3. Search Book by Title\n";
        cout << "4. Search Book by Author\n";
        cout << "5. Issue Book\n";
        cout << "6. Return Book\n";
        cout << "7. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
        case 1:
            lib.addBook();
            break;
        case 2:
            lib.displayBooks();
            break;
        case 3:
            lib.searchByTitle();
            break;
        case 4:
            lib.searchByAuthor();
            break;
        case 5:
            lib.issueBook();
            break;
        case 6:
            lib.returnBook();
            break;
        case 7:
            cout << "Exiting Program...\n";
            break;
        default:
            cout << "Invalid Choice!\n";
        }

    } while (choice != 7);

    return 0;
}
