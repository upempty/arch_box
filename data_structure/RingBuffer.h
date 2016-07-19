/* 
Queue with one slot empty at the end
0 1 2 3 4 5 6 7 8 -
---------- -
front     end with one not used

**/
class CircleQueue
{
public:
    CircleQueue() {
       buf = new char [SIZE];
       front = end = 0;
    };

    ~CircleQueue() {
        delete buf;
    } 

    bool is_empty(void) {
        return (front == end);
    };
    
    bool is_full(void) {
        return ((end + 1) % SIZE == front);
    };
    
    bool push(char c) {
        if (is_full())
            return false;
        buf[end] = c;
        end = (end + 1) % SIZE;
        std::cout<<"after push:"<<c<<", front="<<front<<" end="<<end<<std::endl;
        return true;
    };
    
    char pop(void) {
        if (is_empty())
            return '#';
        char ret = buf[front];
        front = (front + 1) % SIZE;
        return ret;
    }; 
   
    char *get_all() {
        if (is_empty())
            return NULL;
        char *ret = new char [SIZE];
        int tmp_end = end;
        int count = front < end ? end - front : SIZE - front + end;
        if (front < end) {
            for (int i = front; i < end; i++) {
                ret[i - front] = buf[i];
            }
        }
        else {
           for (int i = front; i < SIZE; i++) {
               ret[i - front] = buf[i];
           }
           for (int i = 0; i < end; i++) {
               ret[SIZE - front + i] = buf[i];
           } 
        }
        front = tmp_end;
        return ret;

    }; 
private:
    const static int SIZE = 1000;
    char *buf;
    int front;
    int end;
};
