//
// Created by misha on 02.10.2020.
//

#ifndef VIDEO_RECIEVER_BUFFER_H
#define VIDEO_RECIEVER_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define DEBUG_BUFFER 1
#ifdef DEBUG_BUFFER
#include <mutex>
#endif

#define FFMIN(a, b) ((a) > (b) ? (b) : (a))
#define BUFFER_SAFE_DEC(x, LIMIT) ((x + LIMIT - 1) % LIMIT)
#define CHECK_OVERFLOW(x, LIMIT) (x % LIMIT)
#define NEGATIVE_SAFE_DEC(x) (x >= 1 ? (x - 1) : 0)

struct Buffer
{
    Buffer(uint8_t *data, size_t size) : _data(data),
                                         _size(size),
                                         _empty(size),
                                         _write_cur(0),
                                         _read_cur(0),
                                         _edge(size),
                                         _min_val(size / 10)
    {
        //todo: assert size <= 0
    }

    uint8_t *get_free_buffer(uint32_t &size)
    {
        update_cursors();
        size = _empty;
        return &_data[_write_cur];
    }

    uint8_t *get_data_buffer(uint32_t &size)
    {
        update_cursors();
        size = _to_read;
        return &_data[_read_cur];
    }

    void update_cursors()
    {

        _read_edge = (_read_cur > _write_cur) ? _edge : _size;

        //_read_edge = NEGATIVE_SAFE_DEC(_read_edge);

        auto to_read_edge = (_read_cur > _write_cur) ? _edge : _write_cur;
        _to_read = to_read_edge - _read_cur;

        auto write_border = BUFFER_SAFE_DEC(_read_cur, _size);

        bool normal_position = _write_cur > write_border;

        auto capacity_edge = normal_position ? (_size + write_border) : write_border;
        _capacity = capacity_edge - _write_cur;

        auto empty_edge = normal_position ? _size : write_border;
        _empty = empty_edge - _write_cur;

        if ((_empty < _min_val) &&
            (normal_position) &&
            (_empty < write_border))
        {
            _edge = _write_cur;
            _write_cur = 0;
            _empty = write_border;
            _capacity = _empty;
        }

        //        auto delta = _write_cur >= _read_cur ? (_write_cur - _read_cur) : (_read_cur - _write_cur);
        //        _count = _write_cur >= _read_cur ? (delta) : (_size - delta);
    }

    void push_size(size_t size)
    {
        update_cursors();

        _write_cur += FFMIN(size, _empty);
        _write_cur = CHECK_OVERFLOW(_write_cur, _size);
        _count += size;
    }

    void pop_size(size_t size)
    {
        update_cursors();

        //        _to_read;
        //
        //        size_t finish = _size;
        //
        //        if (_write_cur > _read_cur) {
        //            finish = _write_cur;
        //        }
        //
        //        size_t max_to_pop = finish - _read_cur;
        //
        //        if (size > max_to_pop) {
        //            size = max_to_pop;
        //        }
        _read_cur += FFMIN(size, _to_read);

        if (_read_cur >= _read_edge)
        {
            _read_cur = 0;
            _edge = _size;
        }
        _count -= size;
    }

    size_t get_count()
    {
        update_cursors();
        return _count;
    }

    size_t get_empty()
    {
        update_cursors();
        return _capacity;
    }

#ifdef DEBUG_BUFFER
    std::mutex mut;
    std::string name;
#endif
    void panic_print()
    {
#ifdef DEBUG_BUFFER
        std::unique_lock<std::mutex> lk(mut);
#endif
        update_cursors();
        // log_message(debug, "----- %s ------" << name.c_str());
        // log_message(debug, "buffer is broken");
        // log_message(debug, "_size: %lu" << _size);
        // log_message(debug, "_write_cur: %lu" << _write_cur);
        // log_message(debug, "_read_cur: %lu" << _read_cur);
        // log_message(debug, "_to_read: %lu" << _to_read);
        // log_message(debug, "_empty: %lu" << _empty);
        // log_message(debug, "_count: %lu" << _count);
        // log_message(debug, "_edge: %lu" << _edge);
        // log_message(debug, "-----------------------------");
    }

    int write(uint8_t *data, uint32_t len)
    {
        int size = _write_without_overlap(data, len);

        if (size < len)
        {
            size += _write_without_overlap(&data[size], len - size);
        }

        if (size < len)
        {
            //TODO: How to handle this hardfault?
            // log_message(error, "Buffer is full and some data is missed");
            panic_print();
        }

        return size;
    }

    int read(uint8_t *data, uint32_t len)
    {

        int size = _read_without_overlap(data, len);

        if (size < len)
        {
            size += _read_without_overlap(&data[size], len - size);
        }

        if (size < len)
        {
            //TODO: How to handle this hardfault?
            //log_message(debug, "Buffer is empty, and it can afford given size");
            panic_print();
        }

        return size;
    }

private:
    int _write_without_overlap(uint8_t *data, uint32_t len)
    {
        uint32_t size;
        uint8_t *ptr = get_free_buffer(size);

        if (!size)
        {
            // log_message(error, "Converter buffer is full");
            //exit(0);
            return 0;
        }

        size = FFMIN(len, size);
        memcpy(ptr, data, size);
        push_size(size);
        return size;
    }

    int _read_without_overlap(uint8_t *data, uint32_t len)
    {
        uint32_t size;
        uint8_t *ptr = get_data_buffer(size);

        if (!size)
        {
            // log_message(debug, "Converter buffer is empty");
            //exit(0);
            return 0;
        }

        size = FFMIN(len, size);
        memcpy(data, ptr, size);
        pop_size(size);
        return size;
    }

    uint8_t *_data;
    size_t _size = 0;
    size_t _empty = 0;
    size_t _capacity = 0;
    size_t _write_cur = 0;
    size_t _read_cur = 0;
    size_t _to_read = 0;
    size_t _count = 0;
    size_t _edge = 0;
    size_t _min_val = 0;
    size_t _read_edge = 0;
};

#endif //VIDEO_RECIEVER_BUFFER_H
