#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <unistd.h>
#include <unordered_map>

struct allocation
{
    std::size_t size; 
    void *space;     
};

struct Node
{
    allocation *alloc;
    Node *next;
};

class LinkedList
{
public:
    Node *head;

    LinkedList() : head(nullptr) {}

    void add(std::size_t size, void *space);
    void remove(void *space);
    void print(const std::string &type, bool isFreeList = false, std::unordered_map<void *, std::size_t> *usedSizeMap = nullptr);
    allocation *firstFit(std::size_t size);
};

LinkedList freeList;
LinkedList allocatedList;
std::stack<void *> allocStack; 
std::vector<std::size_t>  partitionSizes = {32, 64, 128, 256, 512};
std::unordered_map<void *, std::size_t> usedSizeMap; 

void initializePartitionSizes();
void *allocFirstFit(std::size_t chunk_size);
void dealloc(void *chunk = nullptr);
void executeTest(const std::string &filename);
void printResults();

int main(int argc, char *argv[])
{
    if (argc != 2 || (std::string(argv[1]) != "datafile"))
    {
        std::cerr << "Usage: " << argv[0] << " datafile\n";
        exit(EXIT_FAILURE);
    }
    executeTest(argv[1]); 
    printResults();

    return 0;
}

void LinkedList::add(std::size_t size, void *space)
{
    allocation *newAlloc = new allocation{size, space};
    Node *newNode = new Node{newAlloc, head};
    head = newNode;
}

void LinkedList::remove(void *space)
{
    Node **current = &head;
    while (*current)
    {
        if ((*current)->alloc->space == space)
        {
            Node *deleteNode = *current;
            *current = (*current)->next;
            delete deleteNode->alloc;
            delete deleteNode;
            return;
        }
        current = &(*current)->next;
    }
}

void LinkedList::print(const std::string &type, bool isFreeList, std::unordered_map<void *, std::size_t> *usedSizeMap)
{
    Node *current = head;
    std::cout << type << ":\n";
    while (current != nullptr)
    {
        std::cout << "Address: " << current->alloc->space
                  << ", Total Size: " << current->alloc->size;
        if (!isFreeList && usedSizeMap)
        {
            std::cout << ", Used Size: " << (*usedSizeMap)[current->alloc->space]; 
        }
        std::cout << "\n";

        current = current->next;
    }
    std::cout << "\n";
}

allocation *LinkedList::firstFit(std::size_t size)
{
    Node *current = head;
    while (current != nullptr)
    {
        if (current->alloc->size >= size)
        {
            return current->alloc;
        }
        current = current->next;
    }
    return nullptr;
}

void *allocFirstFit(std::size_t chunk_size)
{
    std::size_t alloc_size = 0;
    for (std::size_t size : partitionSizes)
    {
        if (chunk_size <= size)
        {
            alloc_size = size;
            break;
        }
    }

    if (alloc_size == 0)
    {
        std::cerr << "Chunk size exceeded maximum partition size\n";
        exit(EXIT_FAILURE);
    }

    allocation *alloc = freeList.firstFit(alloc_size);
    if (alloc != nullptr)
    {
        void *allocated_space = alloc->space;
        freeList.remove(allocated_space);
        allocatedList.add(alloc_size, allocated_space);
        allocStack.push(allocated_space);             
        usedSizeMap[allocated_space] = chunk_size; 
        return allocated_space;
    }

    void *new_space = sbrk(alloc_size);
    if (new_space == (void *)-1)
    {
        std::cerr << "Failed to create new space\n";
        exit(EXIT_FAILURE);
    }

    allocatedList.add(alloc_size, new_space);
    allocStack.push(new_space);             
    usedSizeMap[new_space] = chunk_size; 
    return new_space;
}

void dealloc(void *chunk)
{
    if (chunk == nullptr)
    {
        if (allocStack.empty())
        {
            std::cerr << "No memory to deallocate\n";
            exit(EXIT_FAILURE); 
            return;
        }
        void *last_alloc = allocStack.top();
        allocStack.pop();
        freeList.add(allocatedList.head->alloc->size, last_alloc); 
        allocatedList.remove(last_alloc);
        usedSizeMap.erase(last_alloc);
        return;
    }

    Node *current = allocatedList.head;
    while (current != nullptr)
    {
        if (current->alloc->space == chunk)
        {
            freeList.add(current->alloc->size, chunk);  
            allocatedList.remove(current->alloc->space); 
            return;
        }
        current = current->next;
    }
    std::cerr << "Fatal error: attempting to free memory not allocated.\n";
    exit(EXIT_FAILURE);
}

void executeTest(const std::string &filename)
{
    std::ifstream infile(filename);
    if (!infile)
    {
        std::cerr << "Could not open the file " << filename << "\n";
        return;
    }
    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        std::string function;
        iss >> function;
        if (function == "alloc:")
        {
            std::size_t size;
            iss >> size;
            allocFirstFit(size);
        }
        else if (function == "dealloc")
        {
            dealloc();
        }
    }
}

void printResults()
{
    allocatedList.print("Allocated List", false, &usedSizeMap);
    freeList.print("Free List", true);
}
