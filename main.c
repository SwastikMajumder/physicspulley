#include <string.h>
#include <stdio.h>
#include <SDL.h>
#include <math.h>

#define WINDOW_WIDTH 1000
#define RADIUS 250

struct COLOR_MIX {
	unsigned char RED;
	unsigned char GREEN;
	unsigned char BLUE;
};

void draw_buffer(SDL_Renderer *renderer, struct COLOR_MIX *frame_buffer){
	SDL_Texture *Tile = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, RADIUS*2, RADIUS*2);
	unsigned char *bytes;
    int pitch;
    SDL_LockTexture(Tile, NULL, (void **)&bytes, &pitch);
    int i, j;
    for (i=0; i<RADIUS*2; ++i){
    	for (j=0; j<RADIUS*2; ++j){
    		bytes[(i*RADIUS*8+j*4)] = 255;
    		bytes[(i*RADIUS*8+j*4) + 1] = frame_buffer[j*RADIUS*2+i].BLUE;
    		bytes[(i*RADIUS*8+j*4) + 2] = frame_buffer[j*RADIUS*2+i].GREEN;
    		bytes[(i*RADIUS*8+j*4) + 3] = frame_buffer[j*RADIUS*2+i].RED;
    	}
    }
    SDL_UnlockTexture(Tile);
    SDL_Rect destination = {0, 0, RADIUS*2, RADIUS*2};
    SDL_RenderCopy(renderer, Tile, NULL, &destination);
    SDL_RenderPresent(renderer);
}

void drawBackground(struct COLOR_MIX *frame_buffer){
    int i;
    int j;
    for (i=0; i<RADIUS*2; ++i){
        for (j=0; j<RADIUS*2; ++j){
            frame_buffer[j*RADIUS*2+i].RED = 255;
            frame_buffer[j*RADIUS*2+i].BLUE = 255;
            frame_buffer[j*RADIUS*2+i].GREEN = 255;
        }
    }
}

void drawBox(struct COLOR_MIX *frame_buffer, int length, int height){
    int i;
    int j;
    for (i=0; i<height; ++i){
        for (j=0; j<length; ++j){
            int x=i+RADIUS;
            int y=j+RADIUS;
            frame_buffer[y*RADIUS*2+x].RED = 0;
            frame_buffer[y*RADIUS*2+x].BLUE = 255;
            frame_buffer[y*RADIUS*2+x].GREEN = 0;
        }
    }
}

void rotate_box(float x, float y, float a, float b, float angle){
    float p = a * cos(angle) - b * sin(angle);
    float q = a * sin(angle) + b * cos(angle);
    //printf("%.3f %.3f\n", p + x, q + y);
}

#define PULLEY_OBJ 1
#define MASS_OBJ 2

#define SYSTEM_SIZE 4

struct PULLEY {
    int id;
    int cid[2];
};

struct MASS {
    int id;
    float objmass;
};

struct STORE {
    int obj_type;
    int check;
    void *obj;
};

struct EQUATION {
    int nvar;
    int varid[3];
    float coeff[3];
};

void printeq(struct EQUATION *eq, int *size){
    int i;
    for (i=0; i<(*size); ++i){
        int j;
        for (j=0; j<eq[i].nvar; ++j){
            printf("%3.2f(%d)", eq[i].coeff[j], eq[i].varid[j]);
            if (j == eq[i].nvar - 1){
                printf("+%3.2f", eq[i].coeff[j+1]);
                printf("=0.00\n");
            } else {
                printf("+");
            }
        }
    }
}

void add(struct EQUATION *eq, int length, int idx, int idy, int idz, float a, float b, float c, float d, int *size){
    struct EQUATION neq;
    neq.nvar = length;
    neq.varid[0] = idx;
    neq.varid[1] = idy;
    neq.varid[2] = idz;
    neq.coeff[0] = a;
    neq.coeff[1] = b;
    neq.coeff[2] = c;
    neq.coeff[3] = d;
    memcpy(eq+(*size), &neq, sizeof(struct EQUATION));
    (*size)++;
}


void rem(struct EQUATION *eq, int index, int *size){
    int i;
    for (i=index+1; i<(*size); ++i){
        memcpy(eq+i-1, eq+i, sizeof(struct EQUATION));
    }
    (*size)--;
}

int find(struct STORE *system, int id, int exclude){
    int i;
    for (i=0; i<SYSTEM_SIZE; ++i){
        if (i == exclude) continue;
        if (system[i].obj_type == PULLEY_OBJ){
            struct PULLEY *p = (struct PULLEY *)(system[i].obj);
            if (p->id == id){
                return i;
            }
            int j;
            for (j=0; j<2; ++j){
                if (p->cid[j] == id){
                    return i;
                }
            }
        }
        else if (system[i].obj_type == MASS_OBJ){
            struct MASS *mass = (struct MASS *)(system[i].obj);
            if (mass->id == id){
                return i;
            }
        }
    }
    return -1;
}

void physics(struct STORE *system, struct EQUATION *eq, int *size, int index, float tension){
    int output;
    if (system[index].check == 1) return;
    system[index].check = 1;
    if (system[index].obj_type == PULLEY_OBJ){
        struct PULLEY *p = (struct PULLEY *)(system[index].obj);
        output = find(system, p->id, index);
        if (output == -1){
            add(eq, 1, p->id, 0, 0, 1, 0, 0, 0, size);
            //printf("(%d)=0.00\n", p->id);
        } else {
            physics(system, eq, size, output, tension*2);
        }
        int i;
        for (i=0; i<2; ++i){
            output = find(system, p->cid[i], index);
            if (output == -1){
                add(eq, 1, p->cid[i], 0, 0, 1, 0, 0, 0, size);
                //printf("(%d)=0.00\n", p->cid[i]);
            } else {
                if (system[output].obj_type == PULLEY_OBJ){
                    struct PULLEY *q = (struct PULLEY *)(system[output].obj);
                    if (q->id == p->cid[0] || q->id == p->cid[1]){
                        physics(system, eq, size, output, tension/2);
                    } else {
                        physics(system, eq, size, output, tension);
                    }
                } else {
                    physics(system, eq, size, output, tension);
                }
            }
        }
        add(eq, 3, p->cid[0], p->cid[1], p->id, 1, 1, -2, 0, size);
        //printf("(%d)+(%d)-2.00(%d)=0.00\n", p->cid[0], p->cid[1], p->id);
    }
    else if (system[index].obj_type == MASS_OBJ){
        struct MASS *mass = (struct MASS *)(system[index].obj);
        add(eq, 2, 0, mass->id, 0, tension, -(mass->objmass), mass->objmass*-9.80, 0, size);
        //printf("%3.2f+%3.2f(0)-%3.2f(%d)=0.00\n", mass->objmass*-9.80, tension, mass->objmass, mass->id);
    }
}

int m,n; // original matrix dimensions

float det(float B[m][n]);

float det(float B[m][n]) {
    int row_size = m;
    int column_size = n;
    if (row_size != column_size) {
        printf("DimensionError: Operation Not Permitted \n");
        return 0;
    }
    else if (row_size == 1)
        return (B[0][0]);
    else if (row_size == 2)
        return (B[0][0]*B[1][1] - B[1][0]*B[0][1]);
    else {
        float minor[row_size-1][column_size-1];
        int row_minor, column_minor; //int
        int firstrow_columnindex; //int
        float sum = 0;
        register int row,column; //int
        // exclude first row and current column
        for(firstrow_columnindex = 0; firstrow_columnindex < row_size;
                firstrow_columnindex++) {
            row_minor = 0;
            for(row = 1; row < row_size; row++) {
                column_minor = 0;
                for(column = 0; column < column_size; column++) {
                    if (column == firstrow_columnindex)
                        continue;
                    else
                        minor[row_minor][column_minor] = B[row][column];
                    column_minor++;
                }
                row_minor++;
            }
            m = row_minor;
            n = column_minor;
            if (firstrow_columnindex % 2 == 0)
                sum += B[0][firstrow_columnindex] * det(minor);
            else
                sum -= B[0][firstrow_columnindex] * det(minor);
        }
        return sum;
    }
}

void solve(struct EQUATION *eq, int *size){
    m = n = *size;
    float mat[m][n];
    float mat2[m][n];
    int i;
    int j;
    int k;
    int found;
    for (i=0; i<(*size); ++i){
        for (j=0; j<(*size); ++j){
            found = 0;
            for (k=0; k<eq[j].nvar; ++k){
                if (eq[j].varid[k] == i){
                    found = 1;
                    mat[j][i] = eq[j].coeff[k];
                    break;
                }
            }
            if (found == 0){
                mat[j][i] = 0;
            }
        }
    }
    float D = det(mat);
    int x, y;
    for (i=0; i<(*size); ++i){
        for (x=0; x<(*size); ++x){
            for (y=0; y<(*size); ++y){
                mat2[y][x] = mat[y][x];
            }
        }
        for (j=0; j<(*size); ++j){
            mat2[j][i] = -eq[j].coeff[eq[j].nvar];
        }
        //for (x=0; x<(*size); ++x){
        //    for (y=0; y<(*size); ++y){
        //        printf("%3.2f ", mat2[x][y]);
        //    }
        //    printf("\n");
        //}
        m = n = *size;
        printf("(%d)=%3.2f\n", i, det(mat2)/D);
    }
}

void example(){
    struct PULLEY A;
    struct PULLEY B;
    struct MASS C;
    struct MASS D;
    A.id = 1;
    A.cid[0] = 2;
    A.cid[1] = 3;
    B.id = 4;
    B.cid[0] = 1;
    B.cid[1] = 5;
    C.id = 3;
    D.id = 5;
    C.objmass = 10;
    D.objmass = 8;
    struct STORE *system;
    system = malloc(SYSTEM_SIZE*sizeof(struct STORE));
    system[0].obj = &A;
    system[1].obj = &B;
    system[2].obj = &C;
    system[3].obj = &D;
    int i;
    for (i=0; i<4; ++i){
        system[i].check = 0;
    }
    for (i=0; i<2; ++i){
        system[i].obj_type = PULLEY_OBJ;
    }
    for (i=2; i<4; ++i){
        system[i].obj_type = MASS_OBJ;
    }
    struct EQUATION eq[6];
    int size = 0;
    physics(system, eq, &size, 0, 1);
    solve(eq, &size);
    printeq(eq, &size);
}

int main(int argc, char *argv[]) {
    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(RADIUS*2, RADIUS*2, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    struct COLOR_MIX *frame_buffer = malloc(RADIUS*RADIUS*4 * sizeof(struct COLOR_MIX));

    drawBackground(frame_buffer);

    example();

    drawBox(frame_buffer, 20, 20);
    rotate_box(0, 0, -2, -2, 3.14/4);
    draw_buffer(renderer, frame_buffer);
    while (1) {
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
            break;
    }

	free(frame_buffer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

