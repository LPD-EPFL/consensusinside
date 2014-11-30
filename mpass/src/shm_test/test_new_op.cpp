#include <stdio.h>

class A {
	public:
		void* operator new(unsigned int) {
			printf("new is called\n");
		}

		A(int i) {
		  printf("contructor is called with i=%d\n", i);
		}

};

int main()
{

	A* a = new A(10);

	return 0;
}
