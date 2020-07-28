#ifndef _IMEEK_IDLE_MODE_H_
#define _IMEEK_IDLE_MODE_H_

typedef enum{
    NO_STATE = 0,//无状态
    LEISURE,     //悠闲
    FART,        //放屁
    SLEEP,       //困了
    NAP,         //打盹
    BRUSH_TEETH, //刷牙
    HAVE_A_MEAL, //吃饭
    AMAZE,       //惊讶
    HISS,        //嘘
    DOUBT,       //疑惑
    READ_BOOK,   //看书
}ENUM_IMEEK_STATE;

char *IdleModeAudio[]=
                                                    //行为  概率   音频
{"_20",//悠闲， 3%   开心吹口哨    动作:机器人往前2cm，往后2cm，左转一圈，右转一圈，随机分配
 "_21",//放屁， 5%   放屁的声音
 "_22",//困了， 10%  男孩打哈欠
 "_23",//打盹， 10%  睡觉打呼噜吹口哨
 "_24",//刷牙， 10%   刷牙声
 "_25",//吃饭， 10%   吃东西
 "_26",//惊讶， 10%   惊讶
 "_27",//嘘，   10%   嘘
 "_28",//疑惑， 10%   疑惑
 "_29",//看书， 20%   翻书声
};


#endif

