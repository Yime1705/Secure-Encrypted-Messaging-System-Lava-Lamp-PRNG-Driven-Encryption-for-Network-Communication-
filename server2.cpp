#include <iostream>       // For standard input/output streams
#include <sys/socket.h>   // For socket functions
#include <netinet/in.h>   // For sockaddr_in and network address functions
#include <unistd.h>       // For POSIX functions like close()
#include <cstring>        // For string manipulation functions
#include <fstream>        // For file input/output operations
#include <ctime>          // For generating timestamps

#define PORT 8080         // Port on which the server will listen
#define BUFFER_SIZE 1024  // Buffer size for receiving messages

using namespace std;

// Frame structure to represent data frames
struct Frame {
    string path;          // Path to the image file
    string timestamp;     // Timestamp when the frame was created
    Frame(const string& p) : path(p), timestamp(to_string(time(nullptr))) {} // Constructor initializes with current time
};

// Node structure for the custom queue
struct QueueNode {
    Frame data;           // Frame data stored in the node
    QueueNode* next;      // Pointer to the next node
    QueueNode(const Frame& frame) : data(frame), next(nullptr) {} // Constructor initializes the node
};

// Custom queue implementation using a linked list
class FrameQueue {
private:
    QueueNode *front, *rear; // Pointers to the front and rear of the queue
    size_t currentSize;      // Current size of the queue
    const size_t MAX_SIZE = 10; // Maximum allowed size of the queue

public:
    FrameQueue() : front(nullptr), rear(nullptr), currentSize(0) {} // Constructor initializes an empty queue

    ~FrameQueue() { // Destructor clears the queue
        while (!empty()) {
            pop(); // Remove all elements
        }
    }

    void push(const Frame& frame) { // Adds a frame to the queue
        QueueNode* newNode = new QueueNode(frame); // Create a new node

        if (currentSize >= MAX_SIZE) { // If queue is full, remove the oldest element
            pop();
        }

        if (rear == nullptr) { // If the queue is empty, update both front and rear
            front = rear = newNode;
        } else { // Otherwise, link the new node to the rear
            rear->next = newNode;
            rear = newNode;
        }

        currentSize++; // Increment the queue size
    }

    Frame pop() { // Removes and returns the front frame
        if (empty()) {
            throw runtime_error("Queue is empty"); // Error if queue is empty
        }

        QueueNode* temp = front;      // Store the current front node
        Frame frame = temp->data;     // Extract the frame data
        front = front->next;          // Update the front pointer

        if (front == nullptr) { // If queue becomes empty, set rear to nullptr
            rear = nullptr;
        }

        delete temp; // Free memory of the removed node
        currentSize--; // Decrement the queue size
        return frame;
    }

    bool empty() const { return front == nullptr; } // Check if the queue is empty
    size_t size() const { return currentSize; }     // Return the current size of the queue
};

// AVL Tree Node structure
struct AVLNode {
    string key;           // Key stored in the node
    int height;           // Height of the node for balancing
    AVLNode* left;        // Pointer to left child
    AVLNode* right;       // Pointer to right child

    AVLNode(const string& k) : key(k), height(1), left(nullptr), right(nullptr) {} // Constructor initializes a leaf node
};

// AVL Tree implementation
class AVLTree {
private:
    AVLNode* root; // Root of the AVL tree

    // Helper function to get the height of a node
    int height(AVLNode* node) {
        return node ? node->height : 0;
    }

    // Calculate balance factor of a node
    int getBalance(AVLNode* node) {
        return node ? height(node->left) - height(node->right) : 0;
    }

    // Right rotation for balancing
    AVLNode* rightRotate(AVLNode* y) {
        AVLNode* x = y->left;
        AVLNode* T2 = x->right;

        // Perform rotation
        x->right = y;
        y->left = T2;

        // Update heights
        y->height = max(height(y->left), height(y->right)) + 1;
        x->height = max(height(x->left), height(x->right)) + 1;

        return x;
    }

    // Left rotation for balancing
    AVLNode* leftRotate(AVLNode* x) {
        AVLNode* y = x->right;
        AVLNode* T2 = y->left;

        // Perform rotation
        y->left = x;
        x->right = T2;

        // Update heights
        x->height = max(height(x->left), height(x->right)) + 1;
        y->height = max(height(y->left), height(y->right)) + 1;

        return y;
    }

    // Insert a key into the AVL tree
    AVLNode* insertNode(AVLNode* node, const string& key) {
        // Standard BST insertion
        if (!node) return new AVLNode(key);

        if (key < node->key)
            node->left = insertNode(node->left, key);
        else if (key > node->key)
            node->right = insertNode(node->right, key);
        else
            return node; // Duplicate keys are not allowed

        // Update height of the current node
        node->height = 1 + max(height(node->left), height(node->right));

        // Get the balance factor and balance the tree if needed
        int balance = getBalance(node);

        // Left Left Case
        if (balance > 1 && key < node->left->key)
            return rightRotate(node);

        // Right Right Case
        if (balance < -1 && key > node->right->key)
            return leftRotate(node);

        // Left Right Case
        if (balance > 1 && key > node->left->key) {
            node->left = leftRotate(node->left);
            return rightRotate(node);
        }

        // Right Left Case
        if (balance < -1 && key < node->right->key) {
            node->right = rightRotate(node->right);
            return leftRotate(node);
        }

        return node;
    }

    // Search for a key in the AVL tree
    bool searchKey(AVLNode* node, const string& key) {
        if (!node) return false;
        if (node->key == key) return true;
        if (key < node->key) return searchKey(node->left, key);
        return searchKey(node->right, key);
    }

    // Helper function to destroy the AVL tree
    void destroyTree(AVLNode* node) {
        if (node) {
            destroyTree(node->left);
            destroyTree(node->right);
            delete node;
        }
    }

public:
    AVLTree() : root(nullptr) {} // Constructor initializes an empty tree

    ~AVLTree() { // Destructor clears the tree
        destroyTree(root);
    }

    void insert(const string& key) { // Insert a key into the tree
        root = insertNode(root, key);
    }

    bool contains(const string& key) { // Check if a key exists in the tree
        return searchKey(root, key);
    }
};

// RC4 Encryption/Decryption class
class RC4 {
private:
    unsigned char S[256]; // Permutation array for RC4

    // Key Scheduling Algorithm (KSA) initializes the permutation array
    void KSA(const string& key) {
        for (int i = 0; i < 256; ++i) {
            S[i] = i;
        }

        int j = 0;
        for (int i = 0; i < 256; ++i) {
            j = (j + S[i] + key[i % key.size()]) % 256; // Key-dependent swapping
            swap(S[i], S[j]);
        }
    }

    // Pseudo-Random Generation Algorithm (PRGA) generates the keystream
    string PRGA(const string& data) {
        string result(data.size(), 0); // Result will have the same size as the input
        int i = 0, j = 0;

        for (size_t k = 0; k < data.size(); ++k) {
            i = (i + 1) % 256;
            j = (j + S[i]) % 256;
            swap(S[i], S[j]);
            unsigned char rnd = S[(S[i] + S[j]) % 256]; // Generate pseudo-random byte
            result[k] = data[k] ^ rnd; // XOR input data with the generated byte
        }

        return result; // Return encrypted/decrypted data
    }

public:
    // Encrypt data using the given key
    string encrypt(const string& key, const string& data) {
        KSA(key); // Initialize the permutation array
        return PRGA(data); // Generate encrypted data
    }

    // Decrypt data (same as encrypt for RC4)
    string decrypt(const string& key, const string& data) {
        return encrypt(key, data);
    }
};

// Main function starts here
int main() {
    RC4 rc4;                        // RC4 encryption/decryption instance
    string encryption_key;          // Holds the encryption key
    int server_fd, new_socket;      // Server file descriptor and client socket
    struct sockaddr_in address;     // Server address structure
    int addrlen = sizeof(address);  // Length of the address structure
    char buffer[BUFFER_SIZE] = {0}; // Buffer to hold received data

    // Step 1: Create the server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket failed"); // Error handling if socket creation fails
        exit(EXIT_FAILURE);
    }
    cout << "Socket created successfully.\n";

    // Step 2: Configure server address and bind it to the socket
    address.sin_family = AF_INET;           // IPv4
    address.sin_addr.s_addr = INADDR_ANY;   // Bind to all available interfaces
    address.sin_port = htons(PORT);         // Use the defined port (8080)

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed"); // Error handling if binding fails
        exit(EXIT_FAILURE);
    }
    cout << "Bind successful. Server is listening...\n";

    // Step 3: Start listening for incoming connections
    if (listen(server_fd, 3) < 0) { // Allow up to 3 pending connections
        perror("Listen failed"); // Error handling if listening fails
        exit(EXIT_FAILURE);
    }
    cout << "Server is listening on port " << PORT << "...\n";

    while (true) {
        cout << "Waiting for a connection...\n";

        // Step 4: Accept a client connection
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed"); // Error handling if accepting a connection fails
            continue;
        }
        cout << "Connection established with a client!\n";

        // Step 5: Generate an encryption key (from an image or fallback)
        try {
            encryption_key = "1234567890"; // Placeholder for generated key
            cout << "Using encryption key: " << encryption_key << endl;
        } catch (const exception& e) {
            encryption_key = "1234567890"; // Fallback key if generation fails
            cout << "Using fallback encryption key: " << encryption_key << endl;
        }

        // Step 6: Send the encryption key to the client
        if (send(new_socket, encryption_key.c_str(), encryption_key.length(), 0) < 0) {
            cerr << "Failed to send key to client" << endl;
            close(new_socket);
            continue;
        }

        // Step 7: Handle client communication in a loop
        while (true) {
            // Receive an encrypted message from the client
            memset(buffer, 0, BUFFER_SIZE); // Clear the buffer
            ssize_t bytes_read = read(new_socket, buffer, BUFFER_SIZE - 1);
            if (bytes_read <= 0) {
                cout << "Client disconnected" << endl; // Exit loop if client disconnects
                break;
            }
            buffer[bytes_read] = '\0'; // Null-terminate the received data
            string encrypted_msg(buffer); // Store the encrypted message

            // Decrypt the received message
            string decrypted_msg = rc4.decrypt(encryption_key, encrypted_msg);
            cout << "\nReceived message from client:" << endl;
            cout << "Encrypted: " << encrypted_msg << endl;
            cout << "Decrypted: " << decrypted_msg << endl;

            // Prepare a response (echo back the decrypted message)
            string response = decrypted_msg; // Use the decrypted message as a response
            string encrypted_response = rc4.encrypt(encryption_key, response);

            // Send the encrypted response to the client
            if (send(new_socket, encrypted_response.c_str(), encrypted_response.length(), 0) < 0) {
                cerr << "Failed to send response" << endl;
                break;
            }
        }

        // Step 8: Close the client socket
        close(new_socket);
    }

    // Step 9: Close the server socket when shutting down
    close(server_fd);
    return 0;
}
