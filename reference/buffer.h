#pragma once
template<typename T, int SIZE>
class buffer
{
public:
    buffer();

    T operator[](int i) const;

    void insert(T input);

private:
    T bufferArray[SIZE];
    int index = -1;

    void incrementIndex();
    int modNonNegative(int a, int b) const;
};

template<typename T, int SIZE>
buffer<T, SIZE>::buffer()
{
    for (int i = 0; i < SIZE; i++) {
        bufferArray[i] = 0;
    }
}

template<typename T, int SIZE>
T buffer<T, SIZE>::operator[](int i) const
{
    if (i < 0 || i >= SIZE) {
        // TODO write Serial.Print function giving "Error: buffer index out of range"
        return T();
    }
    return bufferArray[modNonNegative((index - i), SIZE)];
}

template<typename T, int SIZE>
inline void buffer<T, SIZE>::insert(T input)
{
    this->incrementIndex();
    bufferArray[index] = input;
}

template<typename T, int SIZE>
void buffer<T, SIZE>::incrementIndex()
{
    index = (index + 1) % SIZE;
}

template<typename T, int SIZE>
int buffer<T, SIZE>::modNonNegative(int a, int b) const
{
    int x = a % b;
    if (x < 0) {
        return x + b;
    }
    return x;
}