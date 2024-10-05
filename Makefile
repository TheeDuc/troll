TARGETS = firstfit best_fit

all:
	g++ -Wall -Werror -std=c++11 first_fit.cpp -o firstfit
	g++ -Wall -Werror -std=c++11 best_fit.cpp -o bestfit

clean:
	rm -f $(TARGETS)


