#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct node {
    char *value;
    struct node *next;
    struct node *prev;
} node_t;

typedef struct linkedlist {
    node_t *head;
    node_t *tail;
} linkedlist_t;

// Constructors & destructors
linkedlist_t *make_new_list() {
    linkedlist_t *list = malloc(sizeof(linkedlist_t));
    list->head = NULL;
    list->tail = NULL;
    return list;
}

node_t *make_new_node(char *value) {
    node_t *node = malloc(sizeof(node_t));
    node->value = value;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

void free_node(node_t *node) {
    free(node);
}

// Returns the length of a linked list.
int len(linkedlist_t *list) {
    int count = 0;
    node_t *node = list->head;
    while (node != NULL) {
        node = node->next;
        count++;
    }
    return count;
}

// Inserts the string at the spot before `before_node`, or at the end if
// `before_node` is NULL/not in the list. Returns a pointer to the newly
// added node.
node_t *insert(linkedlist_t *list, node_t *before_node, char *value) {
    node_t *new_node = make_new_node(value);
    node_t *node = list->head;
    // List is empty
    if (node == NULL) {
        list->head = new_node;
        list->tail = new_node;
        return new_node;
    }
    while (node != NULL && node != before_node) {
        node = node->next;
    }
    // Add something to end of the list
    if (node == NULL) {
        list->tail->next = new_node;
        new_node->prev = list->tail;
        list->tail = new_node;
    }
    // Add something to beginning of list
    else if (node == list->head) {
        list->head->prev = new_node;
        new_node->next = list->head;
        list->head = new_node;
    }
    // Add something in the middle of the list
    else {
        node_t *new_node_prev = node->prev;
        new_node->prev = new_node_prev;
        new_node_prev->next = new_node;
        new_node->next = node;
        node->prev = new_node;
    }
    return new_node;
}

// Removes a node `delete_node` from the list. Make sure to free its contents
// before calling this function if the string is heap allocated.
void delete(linkedlist_t *list, node_t *delete_node) {
    node_t *node = list->head;
    while (node != NULL && node != delete_node) {
        node = node->next;
    }
    // Node to be deleted not found in list
    if (node == NULL) {
        return;
    }
    node_t *prev = delete_node->prev;
    node_t *next = delete_node->next;
    if (prev != NULL) {
        prev->next = next;
    }
    // Delete node is first element in list
    else {
        list->head = delete_node->next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    // Delete node is last element in list
    else {
        list->tail = delete_node->prev;
    }
    free_node(delete_node);
}

void free_list(linkedlist_t *list) {
    while (list->head != NULL) {
        delete(list, list->head);
    }
    free(list);
}

// Look for a string with equal value in list, and return the node.
// Returns NULL if no such node is found.
node_t *find(linkedlist_t *list, char *str, int str_len) {
    node_t *node = list->head;
    while (node != NULL) {
        char *value = node->value;
        if (value != NULL) {
            int cmp = strncmp(value, str, str_len);
            if (cmp == 0) {
                return node;
            }
        }
        node = node->next;
    }
    return NULL;
}

void tryappend(char *buf, char *toappend, int *copied, int len) {
    // Done copying
    if (*copied == len - 1) {
        return;
    }
    strncpy(&buf[*copied], toappend, len - 1 - *copied);
    buf[len - 1] = '\0';
    *copied = strlen(buf);
}

// Converts list to string a string and stores the string in the buffer.
// If the buffer is overfilled, the result is truncated.
// The buffer is null terminated.
void tostring(linkedlist_t *list, char *buffer, int len) {
    int first = 1;
    int copied = 0;
    tryappend(buffer, "[", &copied, len);
    node_t *node = list->head;
    while (node != NULL && copied < len - 1) {
        if (!first) {
            tryappend(buffer, ", ", &copied, len);
        }
        else {
            first = 0;
        }
        char *value = node->value;
        if (value == NULL) {
            tryappend(buffer, "NULL", &copied, len);
        }
        else {
            tryappend(buffer, "\"", &copied, len);
            tryappend(buffer, value, &copied, len);
            tryappend(buffer, "\"", &copied, len);
        }
        node = node->next;
    }
    tryappend(buffer, "]", &copied, len);
}

// Tests
int assertEqualsString(char *str1, char *str2, int len) {
    if (strncmp(str1, str2, len) == 0) {
        return 0;
    }
    else {
        printf("expected: %s; actual: %s\n", str1, str2);
        return 1;
    }
}

int assertEqualsInt(int i, int j) {
    if (i == j) {
        return 0;
    }
    else {
        printf("expected: %d; actual: %d\n", i, j);
        return 1;
    }
}

int printTest(char *name, int result) {
    printf("Test \"%s\" result code: %d\n", name, result);
    if (result == 0) {
        return 0;
    }
    else {
        return 1;
    }
}

int testInsert() {
    int result = 0;
    linkedlist_t *list = make_new_list();
    insert(list, NULL, "hello");
    insert(list, NULL, "world");
    char buf[100];
    tostring(list, buf, sizeof(buf));
    char *expected = "[\"hello\", \"world\"]";
    result += assertEqualsString(expected, buf, strlen(expected));
    result += assertEqualsInt(2, len(list));
    free_list(list);
    return printTest("testInsert", result);
}

int testDelete() {
    int result = 0;
    linkedlist_t *list = make_new_list();
    node_t *node_1 = insert(list, NULL, "1");
    insert(list, NULL, "2");
    node_t *node_3 = insert(list, NULL, "3");
    insert(list, node_1, "0");
    delete(list, node_1);
    delete(list, node_3);
    char buf[100];
    tostring(list, buf, sizeof(buf));
    char *expected = "[\"0\", \"2\"]";
    result += assertEqualsString(expected, buf, strlen(expected));
    result += assertEqualsInt(2, len(list));
    free_list(list);
    return printTest("testDelete", result);
}

int testFind() {
    int result = 0;
    linkedlist_t *list = make_new_list();
    node_t *node_1 = insert(list, NULL, "1");
    insert(list, NULL, "2");
    node_t *node_3 = insert(list, NULL, "3");
    insert(list, node_1, "0");
    delete(list, node_1);
    delete(list, node_3);
    node_t *node_5 = insert(list, NULL, "5");
    insert(list, node_5, "4");
    node_t *node_2 = find(list, "2", strlen("2"));
    result += assertEqualsString("0", node_2->prev->value, strlen("0"));
    result += assertEqualsString("4", node_2->next->value, strlen("4"));
    delete(list, node_2);
    char buf[100];
    tostring(list, buf, sizeof(buf));
    char *expected = "[\"0\", \"4\", \"5\"]";
    result += assertEqualsString(expected, buf, strlen(expected));
    result += assertEqualsInt(3, len(list));
    free_list(list);
    return printTest("testDelete", result);
}

int main() {
    int failCount = 0;
    failCount += testInsert();
    failCount += testDelete();
    failCount += testFind();
    printf("%d tests failing.\n", failCount);
}
