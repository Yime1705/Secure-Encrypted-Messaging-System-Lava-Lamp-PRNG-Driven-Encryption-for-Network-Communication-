#include <iostream>       
#include <sys/socket.h>   // For socket functions
#include <arpa/inet.h>    // For sockaddr_in and inet functions
#include <unistd.h>       // For close() and other POSIX system calls
#include <string.h>       // For string manipulation functions
#include <fstream>        // For file input/output operations
#include <ctime>          // For generating timestamps

using namespace std;

// Frame structure to represent a data frame
struct Frame {
    string path;          // Path of the image file
    string timestamp;     // Timestamp of the frame's creation
    Frame(const string& p) : path(p), timestamp(to_string(time(nullptr))) {} // Constructor initializes with current time
};

// Node structure for the queue implementation
struct QueueNode {
    Frame data;           // Frame data stored in the node
    QueueNode* next;      // Pointer to the next node
    QueueNode(const Frame& frame) : data(frame), next(nullptr) {} // Constructor initializes the node with data
};

// A custom Queue implemented using a linked list
class FrameQueue {
private:
    QueueNode *front, *rear;  // Pointers to the front and rear of the queue
    size_t currentSize;       // Current size of the queue
    const size_t MAX_SIZE = 10; // Maximum allowed size of the queue

public:
    FrameQueue() : front(nullptr), rear(nullptr), currentSize(0) {} // Constructor initializes an empty queue
    
    ~FrameQueue() {             // Destructor clears the queue
        while (!empty()) {
            pop();              // Remove elements until empty
        }
    }

    void push(const Frame& frame) { // Adds a new frame to the queue
        QueueNode* newNode = new QueueNode(frame); // Create a new node
        
        if (currentSize >= MAX_SIZE) { // Remove the oldest element if queue is full
            pop();
        }

        if (rear == nullptr) { // If the queue is empty, update both front and rear
            front = rear = newNode;
        } else {               // Otherwise, link the new node to the rear
            rear->next = newNode;
            rear = newNode;
        }
        
        currentSize++; // Increment the queue size
    }

    Frame pop() {       // Removes and returns the front frame
        if (empty()) {
            throw runtime_error("Queue is empty"); // Error if queue is empty
        }

        QueueNode* temp = front;      // Store the current front node
        Frame frame = temp->data;     // Extract the frame data
        front = front->next;          // Update the front pointer

        if (front == nullptr) {       // If queue becomes empty, set rear to nullptr
            rear = nullptr;
        }

        delete temp;       // Free memory of the removed node
        currentSize--;     // Decrement the queue size
        return frame;
    }

    bool empty() const { return front == nullptr; } // Checks if the queue is empty
    size_t size() const { return currentSize; }     // Returns the current size of the queue
};

// RC4 encryption/decryption implementation
class RC4 {
private:
    unsigned char S[256]; // Permutation array for RC4 algorithm

    void KSA(const string& key) { // Key-scheduling algorithm to initialize S
        for (int i = 0; i < 256; ++i) {
            S[i] = i;
        }

        int j = 0;
        for (int i = 0; i < 256; ++i) {
            j = (j + S[i] + key[i % key.size()]) % 256; // Key-dependent swapping
            swap(S[i], S[j]);
        }
    }

    string PRGA(const string& data) { // Pseudo-random generation algorithm for encryption/decryption
        string result(data.size(), 0); // Initialize result string with the same size as input
        int i = 0, j = 0;

        for (size_t k = 0; k < data.size(); ++k) {
            i = (i + 1) % 256;
            j = (j + S[i]) % 256;
            swap(S[i], S[j]); // Swap values in S array
            unsigned char rnd = S[(S[i] + S[j]) % 256]; // Generate a pseudo-random byte
            result[k] = data[k] ^ rnd; // XOR the data with the pseudo-random byte
        }

        return result; // Return the encrypted/decrypted result
    }

public:
    string encrypt(const string& key, const string& data) { // Encrypts the data using the given key
        KSA(key); // Initialize S array using the key
        return PRGA(data); // Encrypt the data using PRGA
    }

    string decrypt(const string& key, const string& data) { // Decrypts the data (same as encryption for RC4)
        return encrypt(key, data);
    }
};

class ImageProcessor {
private:
    FrameQueue frameQueue; // Queue to manage frames generated from image files
    
    // Function to generate a numeric string based on image content
    string GenerateNumberFromImage(const string& imagePath) {
        ifstream imageFile(imagePath, ios::binary); // Open image file in binary mode
        if (!imageFile) { // Check if file could not be opened
            cerr << "Error: Unable to open image file: " << imagePath << endl;
            return "";
        }

        unsigned long hash = 5381; // Initialize hash with a large prime number
        char c;
        while (imageFile.get(c)) { // Read each byte from the file
            hash = ((hash << 5) + hash) + static_cast<unsigned char>(c); // Update hash using a variation of DJB2 algorithm
        }
        imageFile.close(); // Close the file

        string number;
        unsigned long tempHash = hash;

        // Convert the hash to a 10-digit numeric string
        for (int i = 0; i < 10; ++i) {
            number = to_string(tempHash % 10) + number;
            tempHash /= 10;
        }

        frameQueue.push(Frame(imagePath)); // Add a new frame to the queue
        return number; // Return the generated numeric string
    }

public:
    // Public method to generate a key from an image file
    string generateKey(const string& imagePath) {
        return GenerateNumberFromImage(imagePath);
    }

    // Check if there are frames in the queue
    bool hasFrames() const {
        return !frameQueue.empty();
    }

    // Retrieve the next frame from the queue
    Frame getNextFrame() {
        return frameQueue.pop();
    }
};

int main() {
    RC4 rc4;                     // RC4 encryption/decryption object
    ImageProcessor imageProcessor; // Image processor object for key generation
    string encryption_key;       // Variable to store the encryption key
    
    // Create a client socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) { // Check for socket creation failure
        cerr << "Could not create socket" << endl;
        return 1;
    }
    cout << "Client socket created" << endl;

    // Configure server address
    struct sockaddr_in server;
    server.sin_family = AF_INET; // IPv4 address family
    server.sin_port = htons(8080); // Set server port to 8080
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address (localhost)

    // Connect to the server
    cout << "Attempting to connect to server..." << endl;
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) { // Check for connection failure
        cerr << "Connection failed: " << strerror(errno) << endl;
        return 1;
    }
    cout << "Connected to server" << endl;

    // Receive encryption key from server
    char key_buffer[1024] = {0}; // Buffer to store the received key
    int key_size = recv(sock, key_buffer, sizeof(key_buffer) - 1, 0); // Receive key from server
    if (key_size < 0) { // Check for receive failure
        cerr << "Failed to receive key: " << strerror(errno) << endl;
        close(sock);
        return 1;
    }
    key_buffer[key_size] = '\0'; // Null-terminate the received key
    encryption_key = string(key_buffer); // Store the key in a string variable
    cout << "Received encryption key from server: " << encryption_key << endl;

    // Generate local encryption key from an image
    try {
        string localKey = imageProcessor.generateKey("opencv_frame_0.png"); // Generate key from image
        cout << "Generated local key from image: " << localKey << endl;
    } catch (const exception& e) {
        cout << "Warning: Could not generate local key from image" << endl;
    }

    while (true) {
        // Get a message from the user
        string message;
        cout << "\nEnter message (or 'exit' to quit): ";
        getline(cin, message);

        if (message == "exit") { // Check if the user wants to exit
            break;
        }

        // Encrypt the message and send it to the server
        string encrypted_message = rc4.encrypt(encryption_key, message); // Encrypt using RC4
        if (send(sock, encrypted_message.c_str(), encrypted_message.length(), 0) < 0) { // Send encrypted message
            cerr << "Send failed: " << strerror(errno) << endl;
            break;
        }
        cout << "Sent encrypted message: " << encrypted_message << endl;

        // Receive the server's encrypted response
        char buffer[2000] = {0}; // Buffer for the response
        int recv_size = recv(sock, buffer, sizeof(buffer) - 1, 0); // Receive response
        if (recv_size < 0) { // Check for receive failure
            cerr << "Receive failed: " << strerror(errno) << endl;
            break;
        }
        buffer[recv_size] = '\0'; // Null-terminate the received response
        
        // Decrypt the server's response
        string encrypted_response(buffer); // Store encrypted response
        string decrypted_response = rc4.decrypt(encryption_key, encrypted_response); // Decrypt the response
        
        // Display the server's response
        cout << "Server reply:" << endl;
        cout << "Encrypted: " << encrypted_response << endl;
        cout << "Decrypted: " << decrypted_response << endl;
    }

    close(sock); // Close the socket
    return 0;
}
