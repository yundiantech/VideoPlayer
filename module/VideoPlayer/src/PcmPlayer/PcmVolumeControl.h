/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef PCMVOLUMECONTROL_H
#define PCMVOLUMECONTROL_H

class PcmVolumeControl
{
public:
    PcmVolumeControl();

    ///buf为需要调节音量的音频数据块首地址指针，size为长度，uRepeat为重复次数，通常设为1，vol为增益倍数,可以小于1
    static void RaiseVolume(char* buf, int size, int uRepeat, double vol);

};




#endif // PCMVOLUMECONTROL_H
