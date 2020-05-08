
#include "showVideoWidget.h"
#include "ui_showVideoWidget.h"

#include <QPainter>
#include <QDebug>
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QFile>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QMouseEvent>

#include <QTimer>
#include <QDrag>
#include <QMimeData>

#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QDateTime>

#include "AppConfig.h"

#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

///用于绘制矩形
//! [3]
static const char *vertexShaderSource =
    "attribute highp vec4 posAttr;\n"
    "attribute lowp vec4 colAttr;\n"
    "varying lowp vec4 col;\n"
    "uniform highp mat4 matrix;\n"
    "void main() {\n"
    "   col = colAttr;\n"
    "   gl_Position = posAttr;\n"
    "}\n";

static const char *fragmentShaderSource =
    "varying lowp vec4 col;\n"
    "void main() {\n"
    "   gl_FragColor = col;\n"
    "}\n";
//! [3]


ShowVideoWidget::ShowVideoWidget(QWidget *parent) :
    QOpenGLWidget(parent),
    ui(new Ui::ShowVideoWidget)
{
    ui->setupUi(this);

    connect(ui->pushButton_close, &QPushButton::clicked, this, &ShowVideoWidget::sig_CloseBtnClick);

    ui->pushButton_close->hide();

    mIsPlaying = false;
    mPlayFailed = false;

    ui->widget_erro->hide();
    ui->widget_name->hide();

    textureUniformY = 0;
    textureUniformU = 0;
    textureUniformV = 0;
    id_y = 0;
    id_u = 0;
    id_v = 0;

    m_pVSHader = NULL;
    m_pFSHader = NULL;
    m_pShaderProgram = NULL;
    m_pTextureY = NULL;
    m_pTextureU = NULL;
    m_pTextureV = NULL;

    m_vertexVertices = new GLfloat[8];

    mVideoFrame.reset();

    m_nVideoH = 0;
    m_nVideoW = 0;

    mPicIndex_X = 0;
    mPicIndex_Y = 0;

    setAcceptDrops(true);

    mCurrentVideoKeepAspectRatio = AppConfig::gVideoKeepAspectRatio;
    mIsShowFaceRect = false;

    mIsCloseAble = true;

    mIsOpenGLInited = false;

    mLastGetFrameTime = 0;

}

ShowVideoWidget::~ShowVideoWidget()
{
    delete ui;
}

void ShowVideoWidget::setIsPlaying(bool value)
{
    mIsPlaying = value;

    FunctionTransfer::runInMainThread([=]()
    {
        if (!mIsPlaying)
        {
            ui->pushButton_close->hide();
        }
        update();
    });

}

void ShowVideoWidget::setPlayFailed(bool value)
{
    mPlayFailed = value;
    FunctionTransfer::runInMainThread([=]()
    {
        update();
    });
}

void ShowVideoWidget::setCameraName(QString name)
{
    mCameraName = name;
    FunctionTransfer::runInMainThread([=]()
    {
        update();
    });
}

void ShowVideoWidget::setVideoWidth(int w, int h)
{
    if (w <= 0 || h <= 0) return;

    m_nVideoW = w;
    m_nVideoH = h;
qDebug()<<__FUNCTION__<<w<<h<<this->isHidden();

    if (mIsOpenGLInited)
    {
        FunctionTransfer::runInMainThread([=]()
        {
            resetGLVertex(this->width(), this->height());
        });
    }

}

void ShowVideoWidget::setCloseAble(bool isCloseAble)
{
    mIsCloseAble = isCloseAble;
}

void ShowVideoWidget::clear()
{
    FunctionTransfer::runInMainThread([=]()
    {
        mVideoFrame.reset();

        mFaceInfoList.clear();

        update();
    });
}

void ShowVideoWidget::enterEvent(QEvent *event)
{
//    qDebug()<<__FUNCTION__;
    if (mIsPlaying && mIsCloseAble)
        ui->pushButton_close->show();
}

void ShowVideoWidget::leaveEvent(QEvent *event)
{
//    qDebug()<<__FUNCTION__;
    ui->pushButton_close->hide();
}

void ShowVideoWidget::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && mIsPlaying)
    {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        ///为拖动的鼠标设置一个图片
//        QPixmap pixMap = QPixmap::grabWindow(this->winId());
        QPixmap pixMap  = this->grab();
//        QPixmap fullScreenPixmap = QPixmap::grabWindow(QApplication::desktop()->winId());
//        QPixmap pixMap = fullScreenPixmap.copy(200,100,600,400);
        QString name = mCameraName;

        if (name.isEmpty())
        {
            name = "drag";
        }
        QString filePath = QString("%1/%2.png").arg(AppConfig::AppDataPath_Tmp).arg(name);
        pixMap.save(filePath);

        QList<QUrl> list;
        QUrl url = "file:///" + filePath;
        list.append(url);
        mimeData->setUrls(list);
        drag->setPixmap(pixMap);

        ///实现视频画面拖动，激发拖动事件
        mimeData->setData("playerid", mPlayerId.toUtf8());
        drag->setMimeData(mimeData);

//        qDebug()<<__FUNCTION__<<"11111";
        drag->start(Qt::CopyAction| Qt::MoveAction);
//        qDebug()<<__FUNCTION__<<"99999";
    }
    else
    {
        QWidget::mouseMoveEvent(event);
    }
}

void ShowVideoWidget::inputOneFrame(VideoFramePtr videoFrame)
{
    FunctionTransfer::runInMainThread([=]()
    {
        int width = videoFrame.get()->width();
        int height = videoFrame.get()->height();

        if (m_nVideoW <= 0 || m_nVideoH <= 0 || m_nVideoW != width || m_nVideoH != height)
        {
            setVideoWidth(width, height);
        }

        mLastGetFrameTime = QDateTime::currentMSecsSinceEpoch();

        mVideoFrame.reset();
        mVideoFrame = videoFrame;

        update(); //调用update将执行 paintEvent函数
    });

}


void ShowVideoWidget::initializeGL()
{
    qDebug()<<__FUNCTION__<<mVideoFrame.get();

    mIsOpenGLInited = true;

    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    //现代opengl渲染管线依赖着色器来处理传入的数据
    //着色器：就是使用openGL着色语言(OpenGL Shading Language, GLSL)编写的一个小函数,
    //       GLSL是构成所有OpenGL着色器的语言,具体的GLSL语言的语法需要读者查找相关资料
    //初始化顶点着色器 对象
    m_pVSHader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    //顶点着色器源码
    const char *vsrc = "attribute vec4 vertexIn; \
    attribute vec2 textureIn; \
    varying vec2 textureOut;  \
    void main(void)           \
    {                         \
        gl_Position = vertexIn; \
        textureOut = textureIn; \
    }";
    //编译顶点着色器程序
    bool bCompile = m_pVSHader->compileSourceCode(vsrc);
    if(!bCompile)
    {
    }
    //初始化片段着色器 功能gpu中yuv转换成rgb
    m_pFSHader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    //片段着色器源码(windows下opengl es 需要加上float这句话)
        const char *fsrc =
    #if defined(WIN32)
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
    #else
    #endif
    "varying vec2 textureOut; \
    uniform sampler2D tex_y; \
    uniform sampler2D tex_u; \
    uniform sampler2D tex_v; \
    void main(void) \
    { \
        vec3 yuv; \
        vec3 rgb; \
        yuv.x = texture2D(tex_y, textureOut).r; \
        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
        rgb = mat3( 1,       1,         1, \
                    0,       -0.39465,  2.03211, \
                    1.13983, -0.58060,  0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    }";
    //将glsl源码送入编译器编译着色器程序
    bCompile = m_pFSHader->compileSourceCode(fsrc);
    if(!bCompile)
    {
    }


    ///用于绘制矩形
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");


#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1
    //创建着色器程序容器
    m_pShaderProgram = new QOpenGLShaderProgram;
    //将片段着色器添加到程序容器
    m_pShaderProgram->addShader(m_pFSHader);
    //将顶点着色器添加到程序容器
    m_pShaderProgram->addShader(m_pVSHader);
    //绑定属性vertexIn到指定位置ATTRIB_VERTEX,该属性在顶点着色源码其中有声明
    m_pShaderProgram->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
    //绑定属性textureIn到指定位置ATTRIB_TEXTURE,该属性在顶点着色源码其中有声明
    m_pShaderProgram->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);
    //链接所有所有添入到的着色器程序
    m_pShaderProgram->link();
    //激活所有链接
    m_pShaderProgram->bind();
    //读取着色器中的数据变量tex_y, tex_u, tex_v的位置,这些变量的声明可以在
    //片段着色器源码中可以看到
    textureUniformY = m_pShaderProgram->uniformLocation("tex_y");
    textureUniformU =  m_pShaderProgram->uniformLocation("tex_u");
    textureUniformV =  m_pShaderProgram->uniformLocation("tex_v");

    // 顶点矩阵
    const GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         -1.0f, 1.0f,
         1.0f, 1.0f,
    };

    memcpy(m_vertexVertices, vertexVertices, sizeof(vertexVertices));

    //纹理矩阵
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    ///设置读取的YUV数据为1字节对其，默认4字节对齐，
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    //设置属性ATTRIB_VERTEX的顶点矩阵值以及格式
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, m_vertexVertices);
    //设置属性ATTRIB_TEXTURE的纹理矩阵值以及格式
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
    //启用ATTRIB_VERTEX属性的数据,默认是关闭的
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    //启用ATTRIB_TEXTURE属性的数据,默认是关闭的
    glEnableVertexAttribArray(ATTRIB_TEXTURE);
    //分别创建y,u,v纹理对象
    m_pTextureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureY->create();
    m_pTextureU->create();
    m_pTextureV->create();
    //获取返回y分量的纹理索引值
    id_y = m_pTextureY->textureId();
    //获取返回u分量的纹理索引值
    id_u = m_pTextureU->textureId();
    //获取返回v分量的纹理索引值
    id_v = m_pTextureV->textureId();
    glClearColor(0.0,0.0,0.0,0.0);//设置背景色-黑色
    //qDebug("addr=%x id_y = %d id_u=%d id_v=%d\n", this, id_y, id_u, id_v);
}

void ShowVideoWidget::resetGLVertex(int window_W, int window_H)
{
    if (m_nVideoW <= 0 || m_nVideoH <= 0 || !AppConfig::gVideoKeepAspectRatio) //铺满
    {
        mPicIndex_X = 0.0;
        mPicIndex_Y = 0.0;

        // 顶点矩阵
        const GLfloat vertexVertices[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
             -1.0f, 1.0f,
             1.0f, 1.0f,
        };

        memcpy(m_vertexVertices, vertexVertices, sizeof(vertexVertices));

        //纹理矩阵
        static const GLfloat textureVertices[] = {
            0.0f,  1.0f,
            1.0f,  1.0f,
            0.0f,  0.0f,
            1.0f,  0.0f,
        };
        //设置属性ATTRIB_VERTEX的顶点矩阵值以及格式
        glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, m_vertexVertices);
        //设置属性ATTRIB_TEXTURE的纹理矩阵值以及格式
        glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
        //启用ATTRIB_VERTEX属性的数据,默认是关闭的
        glEnableVertexAttribArray(ATTRIB_VERTEX);
        //启用ATTRIB_TEXTURE属性的数据,默认是关闭的
        glEnableVertexAttribArray(ATTRIB_TEXTURE);
    }
    else //按比例
    {
        int pix_W = window_W;
        int pix_H = m_nVideoH * pix_W / m_nVideoW;

        int x = this->width() - pix_W;
        int y = this->height() - pix_H;

        x /= 2;
        y /= 2;

        if (y < 0)
        {
            pix_H = window_H;
            pix_W = m_nVideoW * pix_H / m_nVideoH;

            x = this->width() - pix_W;
            y = this->height() - pix_H;

            x /= 2;
            y /= 2;

        }

        mPicIndex_X = x * 1.0 / window_W;
        mPicIndex_Y = y * 1.0 / window_H;

    //qDebug()<<window_W<<window_H<<pix_W<<pix_H<<x<<y;
        float index_y = y *1.0 / window_H * 2.0 -1.0;
        float index_y_1 = index_y * -1.0;
        float index_y_2 = index_y;

        float index_x = x *1.0 / window_W * 2.0 -1.0;
        float index_x_1 = index_x * -1.0;
        float index_x_2 = index_x;

        const GLfloat vertexVertices[] = {
            index_x_2, index_y_2,
            index_x_1,  index_y_2,
            index_x_2, index_y_1,
            index_x_1,  index_y_1,
        };

        memcpy(m_vertexVertices, vertexVertices, sizeof(vertexVertices));

    #if TEXTURE_HALF
        static const GLfloat textureVertices[] = {
            0.0f,  1.0f,
            0.5f,  1.0f,
            0.0f,  0.0f,
            0.5f,  0.0f,
        };
    #else
        static const GLfloat textureVertices[] = {
            0.0f,  1.0f,
            1.0f,  1.0f,
            0.0f,  0.0f,
            1.0f,  0.0f,
        };
    #endif
        //设置属性ATTRIB_VERTEX的顶点矩阵值以及格式
        glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, m_vertexVertices);
        //设置属性ATTRIB_TEXTURE的纹理矩阵值以及格式
        glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
        //启用ATTRIB_VERTEX属性的数据,默认是关闭的
        glEnableVertexAttribArray(ATTRIB_VERTEX);
        //启用ATTRIB_TEXTURE属性的数据,默认是关闭的
        glEnableVertexAttribArray(ATTRIB_TEXTURE);
    }

}

void ShowVideoWidget::resizeGL(int window_W, int window_H)
{
    mLastGetFrameTime = QDateTime::currentMSecsSinceEpoch();

//    qDebug()<<__FUNCTION__<<m_pBufYuv420p<<window_W<<window_H;
    if(window_H == 0)// 防止被零除
    {
        window_H = 1;// 将高设为1
    }
    //设置视口
    glViewport(0, 0, window_W, window_H);

    int x = window_W - ui->pushButton_close->width() - 22;
    int y = 22;
    ui->pushButton_close->move(x, y);

    x = 0;
    y = window_H / 2 - ui->widget_erro->height() / 2;
    ui->widget_erro->move(x, y);
    ui->widget_erro->resize(window_W, ui->widget_erro->height());

    x = 0;
    y = window_H - ui->widget_name->height() - 6;
    ui->widget_name->move(x, y);
    ui->widget_name->resize(window_W, ui->widget_name->height());


    resetGLVertex(window_W, window_H);

}

 void ShowVideoWidget::paintGL()
 {
//     qDebug()<<__FUNCTION__<<mCameraName<<m_pBufYuv420p;

    mLastGetFrameTime = QDateTime::currentMSecsSinceEpoch();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (ui->pushButton_close->isVisible())
        if (!mIsPlaying || !mIsCloseAble)
        {
         ui->pushButton_close->hide();
        }

    if (!mCameraName.isEmpty() && mIsPlaying)
    {
        ui->widget_name->show();

        QFontMetrics fontMetrics(ui->label_name->font());
        int fontSize = fontMetrics.width(mCameraName);//获取之前设置的字符串的像素大小
        QString str = mCameraName;
        if(fontSize > (this->width() / 2))
        {
           str = fontMetrics.elidedText(mCameraName, Qt::ElideRight, (this->width() / 2));//返回一个带有省略号的字符串
        }
        ui->label_name->setText(str);
        ui->label_name->setToolTip(mCameraName);
    }
    else
    {
        ui->widget_name->hide();
    }

    if (mIsPlaying && mPlayFailed)
    {
        ui->widget_erro->show();
    }
    else
    {
        ui->widget_erro->hide();
    }

    ///设置中按比例发生改变 则需要重置x y偏量
    if (mCurrentVideoKeepAspectRatio != AppConfig::gVideoKeepAspectRatio)
    {
        mCurrentVideoKeepAspectRatio = AppConfig::gVideoKeepAspectRatio;
        resetGLVertex(this->width(), this->height());
    }

    ///绘制矩形框
    if (mIsShowFaceRect && !mFaceInfoList.isEmpty())
    {
        m_program->bind();

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        for (int i = 0; i < mFaceInfoList.size(); i++)
        {
            FaceInfoNode faceNode = mFaceInfoList.at(i);
            QRect rect = faceNode.faceRect;

            int window_W = this->width();
            int window_H = this->height();

            int pix_W = rect.width();
            int pix_H = rect.height();

            int x = rect.x();
            int y = rect.y();

            float index_x_1 = x *1.0 / m_nVideoW * 2.0 -1.0;
            float index_y_1 = 1.0 - (y *1.0 / m_nVideoH * 2.0);

            float index_x_2 = (x + pix_W) * 1.0 / m_nVideoW * 2.0 - 1.0;
            float index_y_2 = index_y_1;

            float index_x_3 = index_x_2;
            float index_y_3 = 1.0 - ((y + pix_H) * 1.0 / m_nVideoH * 2.0);

            float index_x_4 = index_x_1;
            float index_y_4 = index_y_3;

            index_x_1 += mPicIndex_X;
            index_x_2 += mPicIndex_X;
            index_x_3 += mPicIndex_X;
            index_x_4 += mPicIndex_X;

            index_y_1 -= mPicIndex_Y;
            index_y_2 -= mPicIndex_Y;
            index_y_3 -= mPicIndex_Y;
            index_y_4 -= mPicIndex_Y;

            const GLfloat vertices[] = {
                index_x_1, index_y_1,
                index_x_2,  index_y_2,
                index_x_3, index_y_3,
                index_x_4,  index_y_4,
            };

//            #7AC451 - 122, 196, 81 - 0.47843f, 0.768627f, 0.317647f
            const GLfloat colors[] = {
                0.47843f, 0.768627f, 0.317647f,
                0.47843f, 0.768627f, 0.317647f,
                0.47843f, 0.768627f, 0.317647f,
                0.47843f, 0.768627f, 0.317647f
            };

            glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
            glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

            glLineWidth(2.2f); //设置画笔宽度

            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }

        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        m_program->release();
    }

    VideoFrame * videoFrame = mVideoFrame.get();

    if (videoFrame != nullptr)
    {
        uint8_t *m_pBufYuv420p = videoFrame->buffer();

        if (m_pBufYuv420p != NULL)
        {
            m_pShaderProgram->bind();

            //加载y数据纹理
            //激活纹理单元GL_TEXTURE0
            glActiveTexture(GL_TEXTURE0);
            //使用来自y数据生成纹理
            glBindTexture(GL_TEXTURE_2D, id_y);
            //使用内存中m_pBufYuv420p数据创建真正的y数据纹理
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW, m_nVideoH, 0, GL_RED, GL_UNSIGNED_BYTE, m_pBufYuv420p);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            //加载u数据纹理
            glActiveTexture(GL_TEXTURE1);//激活纹理单元GL_TEXTURE1
            glBindTexture(GL_TEXTURE_2D, id_u);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW/2, m_nVideoH/2, 0, GL_RED, GL_UNSIGNED_BYTE, (char*)m_pBufYuv420p+m_nVideoW*m_nVideoH);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            //加载v数据纹理
            glActiveTexture(GL_TEXTURE2);//激活纹理单元GL_TEXTURE2
            glBindTexture(GL_TEXTURE_2D, id_v);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW/2, m_nVideoH/2, 0, GL_RED, GL_UNSIGNED_BYTE, (char*)m_pBufYuv420p+m_nVideoW*m_nVideoH*5/4);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            //指定y纹理要使用新值 只能用0,1,2等表示纹理单元的索引，这是opengl不人性化的地方
            //0对应纹理单元GL_TEXTURE0 1对应纹理单元GL_TEXTURE1 2对应纹理的单元
            glUniform1i(textureUniformY, 0);
            //指定u纹理要使用新值
            glUniform1i(textureUniformU, 1);
            //指定v纹理要使用新值
            glUniform1i(textureUniformV, 2);
            //使用顶点数组方式绘制图形
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            m_pShaderProgram->release();

        }
    }

}
