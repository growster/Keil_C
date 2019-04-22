#include <BasicSet.c>
#define TFT_Width       128
#define TFT_Height      160

unsigned int xdata Area[20];          //追踪背景
unsigned int xdata groundx[20];       //背景
unsigned char data trackSquare[6];    //追踪区域
unsigned char data tetris[5] = {0x00,0x06,0x04,0x04,0};       //低四位存行数据
unsigned char count = 1, Down_Flag = 0, Move_flag = 0;        //定时器T0计数位，下降和移动标志
unsigned char square_x = 3, square_y = 0, rotate = 0;         //初始位置，旋转位置
unsigned int code tetrisData[6][4] = {                        //方块数据
  {0x0c88,0x08e0,0x0226,0x0e20},
  {0x06c0,0x08c4,0x06c0,0x08c4},
  {0x0c60,0x04c8,0x0c60,0x04c8},
  {0x088c,0x02e0,0x0622,0x0e80},
  {0x0e40,0x0464,0x04e0,0x04c4},
  {0x0660,0x0660,0x0660,0x0660}
};
unsigned int code *pTetris = tetrisData[0];    //方块指针
unsigned int code *PTimer;    //随机指针
unsigned char tcount = 0;     //T1计数位
sbit Left = P1^3;             //按键
sbit Right = P1^2;
sbit Rota = P1^1;
sbit fast = P1^0;
void fillRectangle(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned int color){
  if((x >= TFT_Width) || (y >= TFT_Height))
    return;
  if((x + w - 1) >= TFT_Width)
    w = TFT_Width  - x;
  if((y + h - 1) >= TFT_Height)
    h = TFT_Height - y;
  Lcd_SetRegion(x, y, x+w-1, y+h-1);
  for(y = h; y > 0; y--) {
    for(x = w; x > 0; x--) {
      Lcd_WriteData_16(color);
    }
  }
}
// 8X8 square
void fillPoint(unsigned char x, unsigned char y, unsigned int color ) {
  unsigned char i;
  x *= 8;
  y *= 8;
  Lcd_SetRegion(x, y, x+7, y+7);
  for(i = 64; i > 0; i-- )
    Lcd_WriteData_16(color);
}

//数据储存结构：一个16位int类型代表一行
//高位为1代表左墙壁，低三位置1代表右墙壁
/*  墙    1  2  3  4  5  6  7  8  9  10 11   墙14
    11 |  x  x  x  x  x  x  x  x  x  x  x  | 111      0xc007
*/
void trackSquare_Read(unsigned char x, unsigned char y){
  unsigned int xdata *p = Area;
  unsigned char i;
  p += y;           //数据开始行
  for(i = 0; i < 6; i++){
    trackSquare[i] = (*p >> (9-x)); //取出6位数据，右移 14-6+1-x=9-x
    p++;
  }
}

void trackSquare_Write(unsigned char x, unsigned char y){
  unsigned int xdata *p = Area;
  unsigned char i;
  unsigned int a, and = 0x003f;         //  低六位置1
  p += y;
  and <<= (9-x);                        //左移对位
  for(i = 0; i < 6; i++ ){
    a = trackSquare[i] << (9-x);
    *p &= ~and;                         //取反相与，清空Area中的tracksquare区域
    *p |= a;                            //与 新的数据
    p++;
  }

}
void tetris_Storage(unsigned char x, unsigned char y, unsigned char *p){
  unsigned int xdata *pgroundx = groundx;
  unsigned char i;
  unsigned int a;
  pgroundx += (y);
  for(i = 0; i < 6; i++ ){
    a = *p & 0x3f;                   //屏蔽高两位
    a <<= (9-x);
    *pgroundx |= a;                  //写入上一次tracksquare中的数据
    p++;
    pgroundx++;
  }
/*  a = 0x8000;                     //debug
  for(i = 0; i < 16;i++){
    if(groundx[16] & a)
      fillPoint(i, 0, RED);
    a >>= 1;
  }
*/
}

//Show trackSquare
unsigned char showTrackSquare_Down(unsigned char x, unsigned char y, unsigned char direction){
  signed char i,j;
  unsigned char data temptrack[6],aa[6];
  unsigned char data *pTrack =trackSquare;
  unsigned char data *pTemp = temptrack;
  unsigned char row;
  unsigned int  row_int;
  for(i = 0; i < 6; i++)
    temptrack[i] = *pTrack++; //上次记录数据暂存

  for(i = 0; i < 6; i++)
    trackSquare[i] = groundx[y+i] >> (9-x) & 0x3f; //背景数据写入tracksquare

  if(direction == 0)                               //下降
    for(i = 0; i < 4; i++){
      row = tetris[i] << 1;
      if(tetris[i+1] & trackSquare[4-i] >> 1){    //tracksquare 低6位中间4位保存数据，而teteris低四位保存数据， 右移一位
        tetris_Storage(x,y,temptrack);            //判断是否触底，Tetris第1行与tracksquare第4行对位判断
        square_y = 0;     //初始化y坐标
        square_x = 3;     //初始化x坐标
        pTetris = PTimer; //定时器随机指针赋值给pTetris
        row_int = *pTetris;
        for(i = 0; i < 4; i++){
          tetris[3-i] = row_int & 0x0f;            //屏蔽高位，将数据写入tetris
          row_int >>= 4;
        }
        return 1;
      }
      trackSquare[5-i] |= row;                     //track区域与俄罗斯方块相或,保存这次移动操作的数据
    }
  else
    if(direction == 1)                             //左移
      for(i = 0; i < 4; i++){
        row = tetris[i] << 2;
        if(tetris[i] & trackSquare[4-i] >> 2)
          return 1;
        trackSquare[4-i] |= row;
      }
    else
      if(direction == 2)                           //右移
        for(i = 0; i < 4; i++){
          row = tetris[i];
          if(tetris[i] & trackSquare[4-i])
            return 1;
          trackSquare[4-i] |= row;
        }
      else
        if(direction == 3){                        //旋转
          row_int = *(pTetris + rotate);
          rotate++;
          if(rotate > 3)
            rotate = 0;
          for(i = 0; i < 4; i++){
            tetris[3-i] = row_int & 0x0f;          //屏蔽高位，将数据写入tetris
            row_int >>= 4;
          }
          for(i = 0; i < 4; i++){
            row = tetris[i] << 1;
            if(tetris[i] & trackSquare[4-i] >> 1){ //判断是否可以旋转，对应行相与
              rotate--;
              row_int = *(pTetris + rotate);
              for(i = 0; i < 4; i++){
                tetris[3-i] = row_int & 0x0f;      //与值为真返回前一次的数据
                row_int >>= 4;
              }
              return 1;
            }
            trackSquare[4-i] |= row;
          }
        }
        else
          return 1;
  pTemp = &temptrack[0];
  pTrack = trackSquare;
  for(i = 0; i < 6; i++){
    aa[i] = *pTrack ^ *pTemp;
    pTrack++;
    pTemp++;                               //异或得到改变位存入aa
  }

  for(i = 0; i < 6; i++){                 //逐行扫描
    if(aa[i]){                            //异或值为真，改变颜色
      row = 0x20;
      for(j = 0; j < 6; j++){             //行内扫描
        if(aa[i] & row ){                 //按位判断
          if(temptrack[i] & row)          //原来的值为真，改成背景色（黑色），否则填充颜色
            fillPoint(x+j+1,y+i,BLACK);
          else
            fillPoint(x+j+1,y+i,RED);
        }
        row >>= 1;
      }
    }
  }
  return 0;
}

//一行填满消除
void tetris_Clear(void)
{
  unsigned int xdata *pRead = groundx;
  unsigned int xdata *pWrite;
  unsigned int xdata groundx_bak[20],aa[20];
  unsigned char i,co = 0;
  unsigned int row_int;
  for(i = 0; i < 20; i++)
    groundx_bak[i] = *pRead++;
  pRead = &groundx_bak[18];     //指向倒数第2行,读取bak数据
  pWrite = &groundx[18];        //指向倒数第2行，写入groundx
  for(i = 0; i < 18; i++){
    if(*pRead == 0xffff)
      pRead--;                  //跳过这一行
    *pWrite = *pRead;           //写入数据
    if(*pRead == 0)
      *pWrite = 0;
    pRead--;
    pWrite--;
  }
  if(pRead == pWrite)
    return;                           //指向地址相同，返回
  pRead = groundx;                    //消除0xffff后的数据
  pWrite = groundx_bak;
  for(i = 0; i < 20; i++)
    aa[i] = *pRead++ ^ *pWrite++;     //异或值存入aa,pwrite
  pRead = groundx_bak;
  pWrite = aa;
  for(i = 0; i < 20; i++){            //行扫描
    if(*pWrite){                      //空行判断
      row_int = 0x8000;
      for(co = 0; co < 20; co++){     //列扫描
        if(*pWrite & row_int){        //异或值为真，改变数据
          if(*pRead & row_int)        //原来是1改成黑色，原来是0改成红色
          fillPoint(co, i, BLACK);
          else
          fillPoint(co, i, RED);
        }
      row_int >>= 1;
      }
    }
    pWrite++;
    pRead++;
  }
}


void main(void)
{
  unsigned char i;
  PT0 = 1;
  TMOD = 0x11;
  TH0 = 0x3C;
  TL0 = 0xB0;
  TH1 = 0xa6;
  TL1 = 0x28;
  EA = 1;
  ET0 = 1;
  ET1 = 1;
  TR0 = 1;                  //打开定时器0，定时50ms
  TR1 = 1;                  //打开T1，定时23ms
  P1 = 0x0f;                //矩阵键盘接P1口，赋初值
  for(i = 0; i < 20; i++){  //左右边界
    groundx[i] = 0xc007;
    Area[i]=0xc007;
  }
  groundx[19] = 0xffff;     //下边界
  Area[19] = 0xffff;
  lcd_initial();            //液晶屏初始化
  bl=1;                     //背光采用IO控制，也可以直接接到高电平常亮
  //绘出游戏区域
  LCD_Clear(BLACK);		      //黑色背景
  fillRectangle(0,0,16,160,BLUE);
  fillRectangle(0,152,128,8,BLUE);
  fillRectangle(104,0,24,160,BLUE);
  delay(500);

  while(1)
  {
    if(Down_Flag){                                      //downflag定时器T0计时1.5s触发置位，方块下降
      trackSquare_Read(square_x,square_y);              //读取追踪区域数据到Tracksquare
      if(!showTrackSquare_Down(square_x, square_y, 0)){ //判断是否触底，触底则不写入追踪数据，由该函数将数据写入背景区域
        trackSquare_Write(square_x,square_y);           //没有触底，Tracksquare数据写入追踪区域
        square_y++;                   //下移一格
      }
      else{
        tetris_Clear();             //消除
        for(i = 0; i < 20; i++)
        Area[i]=groundx[i];         //同步数据
      }
      Down_Flag = 0;                //清除下降标志
    }
    if(!Left && Move_flag){         //左移判断，同时判断是否允许移动操作，moveflag由T0控制
      delay(20);
      if(!Left){                    //延时消抖，再次判断
        trackSquare_Read(square_x,square_y);
        if(!showTrackSquare_Down(square_x, square_y, 1)){
          trackSquare_Write(square_x,square_y);
          square_x--;
        }
      }
    }
    if(!Right && Move_flag){        //右移判断
      delay(20);
      if(!Right){
        trackSquare_Read(square_x,square_y);
        if(!showTrackSquare_Down(square_x, square_y, 2)){
          trackSquare_Write(square_x,square_y);
          square_x++;
        }
      }
    }
    if(!Rota && Move_flag){        //旋转判断
      delay(20);
      if(!Rota){
        trackSquare_Read(square_x,square_y);
        if(!showTrackSquare_Down(square_x, square_y, 3))
          trackSquare_Write(square_x,square_y);
      }
    }
    if(!fast){                    //加速下降
      delay(20);
      if(!fast)
        Down_Flag = 1;
    }
  }

}

void Timer0() interrupt 1
{
  TH0 = 0x3C;
  TL0 = 0xB0;                //定时50ms
  count++;
  if(count <= 22)           //前22*50=1.1s可以进行左右移动操作
    Move_flag = 1;
  else
    Move_flag = 0;

  if(count >= 30){          //定时1.5s
    count = 0;
    Down_Flag = 1;
  }
}

void Timer1() interrupt 3
{
  TH1 = 0xa6;
  TL1 = 0x28;
  if(tcount > 5)
    tcount = 0;
  PTimer = tetrisData[tcount];
  tcount++;
}
