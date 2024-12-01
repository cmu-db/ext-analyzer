#include "interval.h"
#include "treap-tree.h"
#include "redismodule.h"
#include <float.h>
#include "string.h"
#include "output.h"

TreeNode *newTreeNode(Item *member) {
    TreeNode *node = RedisModule_Alloc(sizeof(TreeNode));
    node->item = member;
    node->priority = random();
    node->parent = NULL;
    node->right = NULL;
    node->left = NULL;
    node->val = member->interval->lowerBound;
    node->maxUpper = member->interval->upperBound;
    node->minLower = member->interval->lowerBound;
    return node;
}

void freeTreeNode(TreeNode *node) {
    if (node != NULL) {
        node->parent = NULL;
        node->left = NULL;
        node->right = NULL;
        node->item = NULL;
        RedisModule_Free(node);
    }
}

double max(double d1, double d2, double d3) {
    double result = d1;
    if (result < d2) {
        result = d2;
    }
    if (result < d3) {
        result = d3;
    }
    return result;
}

double min(double d1, double d2, double d3) {
    double result = d1;
    if (result > d2) {
        result = d2;
    }
    if (result > d3) {
        result = d3;
    }
    return result;
}

void updateBounds(TreeNode *node) {
    if (node != NULL) {
        double leftMinLower = DBL_MAX;
        double rightMinLower = DBL_MAX;
        double leftMaxUpper = -DBL_MAX;
        double rightMaxUpper = -DBL_MAX;
        if (node->left != NULL) {
            leftMinLower = node->left->minLower;
            leftMaxUpper = node->left->maxUpper;
        }
        if (node->right != NULL) {
            rightMinLower = node->right->minLower;
            rightMaxUpper = node->right->maxUpper;
        }
        node->minLower = min(node->minLower, leftMinLower, rightMinLower);
        node->maxUpper = max(node->maxUpper, leftMaxUpper, rightMaxUpper);
    }
}

void bubbleUp(TreeNode **root, TreeNode *node) {
    TreeNode *tempParent, *temp;
    int hasUpDate = 0;
    while ((node->parent != NULL) && (node->parent->priority < node->priority)) {
        tempParent = node->parent;
        node->parent = tempParent->parent;
        tempParent->parent = node;

        if (node->parent != NULL) {
            if (node->parent->left == tempParent) {
                node->parent->left = node;
            } else {
                node->parent->right = node;
            }
        } else {
            *root = node;
        }
        if (node == tempParent->left) {  /* right rotation */
            temp = node->right;
            node->right = tempParent;
            tempParent->left = temp;
        } else { /* left rotation */
            temp = node->left;
            node->left = tempParent;
            tempParent->right = temp;
        }
        if (temp != NULL) {
            temp->parent = tempParent;
        }
        updateBounds(tempParent);
        updateBounds(temp);
        updateBounds(node);
        hasUpDate = 1;
    }
    if (!hasUpDate) {
        updateBounds(node);
    }
}

void insertNode(TreeNode **root, Item *member) {
    TreeNode *buff = *root;
    if (*root == NULL) {
        *root = newTreeNode(member);
        return;
    }
    TreeNode *insertedNode = newTreeNode(member);
    while (buff != NULL) {
        if (buff->val < member->interval->lowerBound) {
            if (buff->left != NULL) {
                buff = buff->left;
            } else {
                insertedNode->parent = buff;
                buff->left = insertedNode;
                break;
            }
        } else {
            if (buff->right != NULL) {
                buff = buff->right;
            } else {
                insertedNode->parent = buff;
                buff->right = insertedNode;
                break;
            }
        }
    }
    bubbleUp(root, insertedNode);
}

void move(TreeNode **root, TreeNode *nodeToMove, TreeNode *current) {
    if (current != NULL) {
        if (current->parent == NULL) {
            *root = nodeToMove;
        } else {
            if (current->parent->left == current) {
                current->parent->left = nodeToMove;
            } else if (current->parent->right == current) {
                current->parent->right = nodeToMove;
            }
        }
    }
}

void deleteNodeFromParent(TreeNode *node) {
    if (node != NULL && node->parent != NULL) {
        if (node->parent->left == node) {
            node->parent->left = NULL;
        } else if (node->parent->right == node) {
            node->parent->right = NULL;
        }
    }
}

TreeNode *searchMinFrom(TreeNode *node) {
    TreeNode *buff = node->left;
    while (buff->left != NULL) {
        buff = buff->left;
    }
    return buff;
}

void deleteNode(TreeNode **root, Item *toDelete) {
    TreeNode *buff = *root;
    while (buff != NULL) {
        if (buff->val < toDelete->interval->lowerBound) {
            buff = buff->left;
        } else {
            buff = buff->right;
        }
        if (buff != NULL && toDelete == buff->item) {
            if (buff->left == NULL && buff->right == NULL) {
                deleteNodeFromParent(buff);
                freeTreeNode(buff);
            } else if (buff->left == NULL && buff->right != NULL) {
                deleteNodeFromParent(buff);
                move(root, buff->right, buff);
                freeTreeNode(buff);
            } else if (buff->left != NULL && buff->right == NULL) {
                deleteNodeFromParent(buff);
                move(root, buff->left, buff);
            } else if (buff->left != NULL && buff->right != NULL) {
                TreeNode *min = searchMinFrom(buff);
                min->parent = buff->parent;
                min->left = buff->left;
                min->right = buff->right;
                freeTreeNode(buff);
            }
            break;
        }
    }
}

void findContains(TreeNode *node, double value, struct RedisModuleCtx *ctx, int *len, long long count, OutputContext *outputContext) {
    if (*len >= count) {
        return;
    }
    if (node != NULL) {
        if (containsValue(node->item->interval, value, 1)) {
            outputContext->output(outputContext, node->item);
            (*len)++;
        }
        if (node->left != NULL && node->left->minLower <= value && value <= node->left->maxUpper) {
            findContains(node->left, value, ctx, len, count, outputContext);
        }
        if (node->right != NULL && node->right->minLower <= value && value <= node->right->maxUpper) {
            findContains(node->right, value, ctx, len, count, outputContext);
        }
    }
}

void findOverlaps(TreeNode *node, Interval *intervalToSearch, struct RedisModuleCtx *ctx, int *len, long long count, OutputContext *outputContext) {
    if (*len >= count) {
        return;
    }
    if (node != NULL) {
        if (overlaps(node->item->interval, intervalToSearch)) {
            outputContext->output(outputContext, node->item);
            (*len)++;
        }
        if (node->left != NULL && (
                (node->left->minLower <= intervalToSearch->lowerBound &&
                 intervalToSearch->lowerBound <= node->left->maxUpper) ||
                (node->left->minLower <= intervalToSearch->upperBound &&
                 intervalToSearch->upperBound <= node->left->maxUpper)
        )) {
            findOverlaps(node->left, intervalToSearch, ctx, len, count, outputContext);
        }
        if (node->right != NULL && (
                (node->right->minLower <= intervalToSearch->lowerBound &&
                 intervalToSearch->lowerBound <= node->right->maxUpper) ||
                (node->right->minLower <= intervalToSearch->upperBound &&
                 intervalToSearch->upperBound <= node->right->maxUpper)
        )) {
            findOverlaps(node->right, intervalToSearch, ctx, len, count, outputContext);
        }
    }
}
