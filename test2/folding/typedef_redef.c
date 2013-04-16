// RUN: %ucc -c %s; [ $? -ne 0 ]
// RUN: %ucc -S -o- %s 2>&1 | %check %s

typedef enum SGridDraggingDecision {
    NoDecision,
    DragCols,
    DragRows
} SGridDraggingDecision;

typedef enum SGridDraggingDecision SGridDraggingDecision; // CHECK: /error: redefinition of typedef from:/
