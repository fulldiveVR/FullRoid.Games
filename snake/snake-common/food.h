#pragma once
#include "config.h"
#include "snake.h"

#define FOOD_RED  0
#define FOOD_BLUE 1

typedef struct {
    int x, y, type;
} FoodItem;

typedef struct {
    FoodItem items[MAX_FOOD];
    int      count;
    int      type_change_counter;
} Food;

void food_reset(Food *f);
void food_spawn(Food *f, const Snake *s);
void food_update(Food *f, const Snake *s);
/* Возвращает 1 если еда съедена, записывает тип в *out_type */
int  food_check_collision(Food *f, Point head, int *out_type);
int  food_occupies(const Food *f, int x, int y);
