#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <dirent.h>
#include <locale.h>
#include <unistd.h>

struct dir{
	char name[255]; 
	int type; // 4 - папка, 10 - ссылка, 8 - файл
	int active;
} folder[255], folder1[255];
int numF = 0, numF1 = 0;
int active = 1, winactive = 1; // порядковый номер в директории, активное окно

int start = 1, end;

char path[255], path1[255];

// функция для копирования файлов
void * copyFile(void *arg){ 
	if(winactive == 1){
		FILE* f = fopen(strcat(strcat(path,"/"),folder[active].name),"r");
		FILE* f1 = fopen(strcat(strcat(path1,"/"),folder[active].name),"w");
		while(!feof(f)){
			char ch;
			fscanf(f,"%c",&ch);
			fprintf(f1,"%c", ch);
		}
		fclose(f);
		fclose(f1);
	} else if(winactive == 2){
		FILE* f = fopen(strcat(strcat(path,"/"),folder1[active].name),"r");
		FILE* f1 = fopen(strcat(strcat(path1,"/"),folder1[active].name),"w");
		while(!feof(f)){
			char ch;
			fscanf(f,"%c",&ch);
			fprintf(f1,"%c", ch);
		}
		fclose(f);
		fclose(f1);
	}
}

// перерисовка экрана
void update_screen(WINDOW *subwnd){
	// заполнение первого дочернего окна
	wattron(subwnd,COLOR_PAIR(2));
	werase(subwnd); // очистка
	box(subwnd, '|','-');
	int i, j, numFT;
	if(numF > end && winactive == 1) numFT = end;
	else if(numF1 > end && winactive == 2) numFT = end;
	else if(winactive == 1) numFT = numF;
	else if(winactive == 2) numFT = numF1;
	for(i = start; i < numFT; i++){
		wmove(subwnd, i - start + 1, 2);
		if(folder[i].active == 1 && winactive == 1 && folder[i].type != 8){
			wattron(subwnd,COLOR_PAIR(1));
			wprintw(subwnd,folder[i].name);
			wattroff(subwnd,COLOR_PAIR(1));
			wattron(subwnd,COLOR_PAIR(2));
		} else if(folder1[i].active == 1 && winactive == 2  && folder1[i].type != 8){
			wattron(subwnd,COLOR_PAIR(1));
			wprintw(subwnd,folder1[i].name);
			wattroff(subwnd,COLOR_PAIR(1));
			wattron(subwnd,COLOR_PAIR(2));
		} else if(folder[i].active == 1 && winactive == 1 && folder[i].type == 8){
			wattron(subwnd,COLOR_PAIR(3));
			wprintw(subwnd,folder[i].name);
			wattroff(subwnd,COLOR_PAIR(3));
			wattron(subwnd,COLOR_PAIR(2));
		} else if(folder1[i].active == 1 && winactive == 2  && folder1[i].type == 8){
			wattron(subwnd,COLOR_PAIR(3));
			wprintw(subwnd,folder1[i].name);
			wattroff(subwnd,COLOR_PAIR(3));
			wattron(subwnd,COLOR_PAIR(2));
		} else if(winactive == 1)
			wprintw(subwnd,folder[i].name);
		else if(winactive == 2)
			wprintw(subwnd,folder1[i].name);
	}
	// обновляем окно
	wprintf(subwnd,path);
	wrefresh(subwnd);	
}

void redir(char direct[255]){
	int i,j;
	// открываем директорию
	if(winactive == 1){
		strcat(path,"/");	
		strcat(path,direct);
		DIR* d = opendir(path);
		struct dirent *dr = NULL;
		numF = 0;
		// считываем все файлы
		while((dr = readdir(d)) != NULL){
			memset(folder[numF].name,0,sizeof(folder[numF].name));
			strcpy(folder[numF].name,dr->d_name);
			folder[numF].type = dr->d_type;
			folder[numF].active = 0;
			numF++;
		}
		closedir(d);
		// сортируем имена файлов
		for(i = 0; i < numF; i++){
			for(j = 0; j < numF-1; j++){
				if(strcmp(folder[j].name,folder[j+1].name) > 0){
					struct dir temp = folder[j];
					folder[j] = folder[j+1];
					folder[j+1] = temp;
				}
			}
		} 
		active = 1;
		folder[active].active = 1;
	} else if(winactive == 2){
		strcat(path1,"/");	
		strcat(path1,direct);
		DIR* d = opendir(path1);
		struct dirent *dr = NULL;
		numF1 = 0;
		// считываем все файлы
		while((dr = readdir(d)) != NULL){
			memset(folder1[numF1].name,0,sizeof(folder1[numF1].name));
			strcpy(folder1[numF1].name,dr->d_name);
			folder1[numF1].type = dr->d_type;
			folder1[numF1].active = 0;
			numF1++;
		}
		closedir(d);
		// сортируем имена файлов
		for(i = 0; i < numF1; i++){
			for(j = 0; j < numF1-1; j++){
				if(strcmp(folder1[j].name,folder1[j+1].name) > 0){
					struct dir temp = folder1[j];
					folder1[j] = folder1[j+1];
					folder1[j+1] = temp;
				}
			}
		} 
		folder1[active].active = 1;
	}
}

int main(){
	setlocale(LC_ALL, "Rus");
	// окно редактора
	WINDOW *wnd, *subwnd, *subwnd1;
	initscr();
	curs_set(0);
	start_color();
	refresh();
	keypad(stdscr, TRUE);
	// инициализация пар цветов (текст - фон)
	init_pair(1, COLOR_WHITE, COLOR_RED); 
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	init_pair(3, COLOR_WHITE, COLOR_GREEN);
	// узнаем размеры окна консоли
	struct winsize w;
    	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	//printf("%d %d\n",w.ws_row,w.ws_col);
	end = w.ws_row-1;
	// задаем главное окно
	wnd = newwin(w.ws_row,w.ws_col,0,0);
 	// задаем под-окна
	subwnd = derwin(wnd, w.ws_row, w.ws_col / 2 - 1, 0, 0);
	subwnd1 = derwin(wnd, w.ws_row, w.ws_col / 2 - 1, 0, w.ws_col / 2);
	wbkgd(subwnd, COLOR_PAIR(2));
	wbkgd(subwnd1, COLOR_PAIR(2));
	redir("");
	winactive = 2;
	redir("");
	update_screen(subwnd);
	update_screen(subwnd1);
	int ch = 0;
	// бесконечный цикл для клавиши
	while(ch != 'q'){
		ch = getch();
		// в зависимости от клавиши
		switch(ch){
			case KEY_UP:{
				if(active < start){ end--; start--; } // прокрутка
				if(active > 1 && winactive == 1){
					folder[active].active = 0;
					active--; 
					folder[active].active = 1;
				} else if(active > 1 && winactive == 2){
					folder1[active].active = 0;
					active--; 
					folder1[active].active = 1;
				}
			} break;
			case KEY_DOWN:{
				if(active >= end - 1 && active < numF - 1 && winactive == 1){ end++; start++; } // прокрутка
				else if(active >= end - 1 && active < numF1 - 1 && winactive == 2){ end++; start++; }
				if(active < numF - 1 && winactive == 1){
					folder[active].active = 0;
					active++; 
					folder[active].active = 1;
				} else if(active < numF1 - 1 && winactive == 2){
					folder1[active].active = 0;
					active++; 
					folder1[active].active = 1;
				}
			} break;
			case '\n':{
				if(winactive == 1 && folder[active].type == 4)
					redir(folder[active].name);
				else if(winactive == 2 && folder1[active].type == 4)
					redir(folder1[active].name);
				active = 1;
			} break;
			case 9: {
				if(winactive == 1) winactive = 2;
				else if(winactive == 2) winactive = 1;
				active = 1;
				if(winactive == 1)
					update_screen(subwnd);
				else if(winactive == 2)
					update_screen(subwnd1);
			} break;
			case KEY_F(5):{
				if(folder[active].type == 8 && winactive == 1){
					pthread_t thread1;
					int result = pthread_create(&thread1, NULL, copyFile, 1);
				} else if(folder1[active].type == 8 && winactive == 2){
					pthread_t thread1;
					int result = pthread_create(&thread1, NULL, copyFile, 1);
				}
			} break;
		}
		if(winactive == 1)
			update_screen(subwnd);
		else if(winactive == 2)
			update_screen(subwnd1);
	}
 	// переписовка окнон
	delwin(subwnd);
	delwin(subwnd1);
	delwin(wnd);
	wmove(stdscr, 8, 1);
	getch();
	endwin();
	exit(EXIT_SUCCESS);
}
