#include <iostream>
#include <set>
#include <assert.h>

struct borderTag {
    size_t size;
    bool is_occupied;
};

struct areaDescriptor {
    borderTag tag;
    areaDescriptor *next;
    areaDescriptor *previous;
};

void *head;
void *start;
void *end;
size_t border_tag_size = sizeof(borderTag);

void clean_pointers(areaDescriptor* area) {
    area->previous = nullptr;
    area->next = nullptr;
}

void remove_node(areaDescriptor *descriptor) {
    if (auto descriptor_previous = descriptor->previous) {
        descriptor_previous->next = descriptor->next;
    }

    if (auto next = descriptor->next) {
        next->previous = descriptor->previous;
    } else {
        head = descriptor->previous;
    }
}

void add_node(areaDescriptor *descriptor) {
    clean_pointers(descriptor);
    if (head) {
        descriptor->previous = (areaDescriptor *)head;
        ((areaDescriptor *)head)->next = descriptor;
    }
    head = descriptor;
}

borderTag create_tag(std::size_t size, bool is_occupied) {
    borderTag tag{};
    tag.size = size;
    tag.is_occupied = is_occupied;

    return tag;
}

void set_tag_to_section(borderTag* tag_start, borderTag tag) {
    *tag_start = tag;
    *(borderTag*)((uint8_t *)tag_start + tag_start->size - border_tag_size) = tag;
}

// Эта функция будет вызвана перед тем как вызывать myalloc и myfree
// используйте ее чтобы инициализировать ваш аллокатор перед началом
// работы.
//
// buf - указатель на участок логической памяти, который ваш аллокатор
//       должен распределять, все возвращаемые указатели должны быть
//       либо равны NULL, либо быть из этого участка памяти
// size - размер участка памяти, на который указывает buf
void mysetup(void *buf, std::size_t size)
{
    auto descriptor = (areaDescriptor*)buf;
    descriptor->next = nullptr;
    descriptor->previous = nullptr;

    auto tag = create_tag(size, false);
    set_tag_to_section((borderTag*)buf, tag);

    head = buf;
    end = (uint8_t *)head + size;
}

void *myalloc_in(void *area, std::size_t size)
{
    areaDescriptor descriptor = *(areaDescriptor*)area;
    if (descriptor.tag.size > size + sizeof(areaDescriptor) + 3 * border_tag_size) {
        auto *pointer = (uint8_t*)area;
        auto allocating_size = size + 2 * border_tag_size;
        auto occupied_tag = create_tag(allocating_size, true);
        auto free_size = descriptor.tag.size - allocating_size;
        auto free_tag = create_tag(free_size, false);

        pointer += free_size;
        set_tag_to_section((borderTag*)pointer, occupied_tag);
        set_tag_to_section((borderTag*)area, free_tag);
        clean_pointers((areaDescriptor *)pointer);

        return pointer + border_tag_size;
    } else if (descriptor.tag.size >= size + 2 * border_tag_size) {
        auto area_descriptor = (areaDescriptor*)area;
        remove_node(area_descriptor);
        clean_pointers(area_descriptor);

        auto tag = create_tag(descriptor.tag.size, true);
        set_tag_to_section((borderTag*)area, tag);

        return (borderTag*)area + 1;
    } else {
        if (descriptor.previous) {
            return myalloc_in(descriptor.previous, size);
        } else {
            return nullptr;
        }
    }
}

// Функция аллокации
void *myalloc(std::size_t size)
{
    if (head) {
        return myalloc_in(head, size);
    } else {
        return nullptr;
    }
}

// Функция освобождения
void myfree(void *p)
{
    auto freed_area_pointer = (borderTag*)p - 1;
    auto left_tag_pointer = (borderTag*)p - 2;
    auto middle_size = freed_area_pointer->size;
    auto right_area_pointer = (borderTag*)((uint8_t*) freed_area_pointer + middle_size);

    if (freed_area_pointer != start && !left_tag_pointer->is_occupied) {
        auto left_area_pointer = (borderTag*)(((uint8_t*)freed_area_pointer) - left_tag_pointer->size);

        if (right_area_pointer != end && !right_area_pointer->is_occupied) {
            remove_node((areaDescriptor*)right_area_pointer);
            auto tag = create_tag(left_area_pointer->size + middle_size + right_area_pointer->size, false);
            set_tag_to_section(left_area_pointer, tag);
        } else {
            auto new_tag = create_tag(left_area_pointer->size + middle_size, false);
            set_tag_to_section(left_area_pointer, new_tag);
        }
    } else {
        if (right_area_pointer != end && !right_area_pointer->is_occupied) {
            remove_node((areaDescriptor*)right_area_pointer);
            auto tag = create_tag(right_area_pointer->size + middle_size, false);
            set_tag_to_section(freed_area_pointer, tag);
            add_node((areaDescriptor *)freed_area_pointer);
        } else {
            auto tag = create_tag(freed_area_pointer->size, false);
            set_tag_to_section(freed_area_pointer, tag);
            add_node((areaDescriptor *)freed_area_pointer);
        }
    }
}

void check_chain() {
    auto current_descriptor = (areaDescriptor*)head;
    while (true) {
        if (current_descriptor && current_descriptor->previous) {
            if (current_descriptor->previous->next != current_descriptor) {
                assert(0);
            }

            current_descriptor = current_descriptor->previous;
        } else {
            break;
        }
    }
}

void check_area(void* start) {
    size_t sum = 0;
    while (start != end) {
        auto pointer = (uint8_t *)start;
        auto left_border_tag = (borderTag *)pointer;
        pointer += left_border_tag->size - border_tag_size;
        auto right_border_tag = (borderTag *)pointer;
        assert(left_border_tag->size == right_border_tag->size);
        assert(left_border_tag->is_occupied == right_border_tag->is_occupied);
        sum += left_border_tag->size;
        start = pointer + border_tag_size;
    }
    assert(sum == 500000);
}

int main() {
    start = malloc(500000);
    mysetup(start, 500000);

    std::set<void*> refs;
    for (int i = 0; i < 100000; ++i) {
        if (rand() % 2) {
            auto kek = rand() % 0 + 16;
            if (auto pointer = myalloc((std::size_t) kek)) {
                refs.insert(pointer);
            }
        } else if (refs.size()) {
            int randomIndex = rand() % refs.size();
            auto itr = refs.begin();
            advance(itr, randomIndex);
            myfree(*itr);
            refs.erase(itr);
        }
//        check_chain();
//        check_area(start);
//        check_area(start);
    }

    while (!refs.empty()) {
        auto element = *refs.begin();
        myfree(element);
        refs.erase(refs.begin());
    }

    printf("%zu\n", ((borderTag*)head)->size);
//    auto end_1 = (uint8_t *)head + 500000;
//    printf("%d\n", (void *)end_1 == end);
//    auto *allocated = (borderTag*)myalloc(100000);
//    printf("%zu\n", (allocated - 1)->size);
//    printf("%zu\n", ((borderTag*)head)->size);
//    auto *allocated_2 = myalloc(100000);
//
//    myfree(allocated);
//    printf("%zu\n", ((borderTag*)head)->size);
//    printf("%zu\n", ((borderTag*)(((uint8_t *)head + ((borderTag*)head)->size) - border_tag_size))->size);
//    auto kek = (borderTag*)((areaDescriptor*)head)->previous;
//    printf("%zu\n", kek->size);
//    printf("%zu\n", ((borderTag*)(((uint8_t *)kek + kek->size) - border_tag_size))->size);
//
//    auto *allocated_3 = myalloc(100000);
//
//    myfree(allocated_2);
//    printf("%zu\n", ((borderTag*)head)->size);
//    printf("%zu\n", ((borderTag*)(((uint8_t *)head + ((borderTag*)head)->size) - border_tag_size))->size);
//    myfree(allocated_3);
//    printf("%zu\n", ((borderTag*)head)->size);
//    printf("%zu\n", ((borderTag*)(((uint8_t *)head + ((borderTag*)head)->size) - border_tag_size))->size);

    return 0;
}
