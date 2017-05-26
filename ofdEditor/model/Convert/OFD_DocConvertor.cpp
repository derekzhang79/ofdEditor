#include "OFD_DocConvertor.h"

#include <QTextBlock>
#include <QVector>
#include "Doc/DocPassage.h"
#include "Doc/DocPage.h"
#include "DataTypes/document/ofd.h"
#include "DataTypes/document/document.h"
#include "DataTypes/page/ct_pages.h"
#include "DataTypes/page/page.h"
#include "Doc/DocTextBlock.h"
#include "Doc/DocBlock.h"
#include "Tool/UnitTool.h"


OFD_DocConvertor::OFD_DocConvertor()
{

}

/**
 * @Author Chaoqun
 * @brief  将ofd格式转换成doc格式
 * @param  OFD * ofd
 * @return DocPassage *
 * @date   2017/05/23
 */
DocPassage *OFD_DocConvertor::ofd_to_doc(OFD *ofd)
{
    DocPassage * passage = NULL;

    try{

        passage = new DocPassage();

        // version
        passage->setVersion(ofd->version);
        passage->setDocType(ofd->doc_type);

        DocBody * docBody = (*ofd->docbodys)[0];        // DocBody
        passage->setDocInfo(*(docBody->doc_info));

        Document * document = (*ofd->docs)[0];          // Document

        CT_CommonData * commonData = document->getCommonData();     // 获取common_data

        //    CT_Pages pages = document->pages;               // 获得文档中的页
        QVector<Page * > * pages = document->pages->pages; // 获得页属性
        for(int i = 0; i <pages->length(); i++)
        {

            //  生成每一页
            DocPage * newPage = this->buildDocPage((*pages)[i]);
            newPage->setVisible(true);
            passage->addPage(newPage);
        }
    }
    catch(...)
    {
        qDebug() << "ofd_to_doc:exception.";
    }

    return passage;
}

DocPage *OFD_DocConvertor::buildDocPage(Page *ct_page)
{
    DocPage * page;
    try
    {
        if(ct_page->area != NULL)
        {
            // 如果定义了纸张尺寸
            ST_Box physical_box = ct_page->area->physical_box;
            page = new DocPage(physical_box.getDeltaX(),
                               physical_box.getDeltaY(),
                               1.0);       // 设置纸张大小
        }
        else
        {
           page = new DocPage();
        }

        page->setVisible(false);        // 先隐藏显示

        // 将每一层加入到页中
        QVector<CT_Layer *>* layers = ct_page->content;     // 获得文章中的层信息
        for(int i = 0; i < layers->size(); i++)
        {
            CT_Layer* layer = (*layers)[i];
            qDebug() << "excute insertLayer: " << i;
            this->insertLayer(page,layer);          // 将该层的内容加入到页面中
        }

    }
    catch(...)
    {
        qDebug() << "OFD_DocConvertor::buildDocPage exception.";
    }


    return page;
}

/**
 * @Author Chaoqun
 * @brief  处理CT_Layer
 * @param  DocPage *page
 * @param  CT_Layer *layer
 * @return void
 * @date   2017/05/23
 */
void OFD_DocConvertor::insertLayer(DocPage *page, CT_Layer *layer)
{
    try
    {

        this->insertPageBlock(page,layer,layer);        // 先将CT_Layer当作PageBlock处理

    }
    catch(...)
    {
        qDebug() << "OFD_DocConvertor::insertLayer has exception";
    }
}

/**
 * @Author Chaoqun
 * @brief  处理插入pageblock
 * @param  DocPage *page
 * @param  CT_Layer *layer
 * @param  CT_PageBlock*
 * @return 返回值
 * @date   2017/xx/xx
 */
void OFD_DocConvertor::insertPageBlock(DocPage *page, CT_Layer *layer, CT_PageBlock *pageBlock)
{

    CT_Layer::LayerType cttype = layer->type;        // 首先获得层
    DocPage::Layer doctype;
    switch (cttype) {
    case CT_Layer::Foreground:
        doctype = DocPage::Foreground;
        break;
    case CT_Layer::Body:
        doctype = DocPage::Body;
        break;
    case CT_Layer::Background:
        doctype = DocPage::Background;
    default:
        break;
    }

    // 处理 CT_Text
    QVector <CT_Text *> * texts = pageBlock->getTextObject();
    for(int i = 0; i<texts->length(); i++)
    {
        this->insertCT_Text(page,doctype,(*texts)[i]);  // 插入文字
    }

    // 处理 CT_Path

    // 处理 CT_Image


}


/**
 * @Author Chaoqun
 * @brief  将文字块插入到页面中
 * @param  DocPage *page
 * @param  DocPage::Layer *layer
 * @param  CT_Text *text
 * @return 返回值
 * @date   2017/05/23
 */
void OFD_DocConvertor::insertCT_Text(DocPage *page, DocPage::Layer layer, CT_Text *text)
{
//    qDebug() << "execute insert CT_Text";

    TextCode* textCode = text->text_code;   // 获得text_code;
    // 先简单点
    QString content = textCode->text;     // 文本内容

//    qDebug() << "insert Content:" << content;

    DocTextBlock *textBlock = new DocTextBlock();
    DocBlock *block = new DocBlock();
    block->setWidget(textBlock);

    ST_Box box = text->boundary;
    block->resize(UnitTool::mmToPixel(box.getDeltaX()),
                  UnitTool::mmToPixel(box.getDeltaY()));
    block->setPos(UnitTool::mmToPixel(box.getX()),
                  UnitTool::mmToPixel(box.getY()));

    QTextCursor cursor = textBlock->textCursor();       // 获得光标

    // 去掉QTextEdit的无用的边框
    textBlock->document()->setDocumentMargin(0);            // document
    QTextFrame * frame = textBlock->document()->rootFrame();
    QTextFrameFormat frameFormat = frame->frameFormat();     // frameFormat
    // 清除没用的边缘距离
    frameFormat.setTopMargin(0);
    frameFormat.setBottomMargin(0);
    frameFormat.setLeftMargin(0);
    frameFormat.setRightMargin(0);
    frameFormat.setPadding(0);

    frame->setFrameFormat(frameFormat);         // 应用Frame格式

    QTextBlockFormat blockFormat = cursor.blockFormat();           // 块格式
    QTextCharFormat charFormat = cursor.charFormat();         // 字符格式
    QFont font = charFormat.font();         // 字体

    // 字号
    font.setPixelSize(UnitTool::mmToPixel(text->size));            // 字号

    // 上下间距
    blockFormat.setTopMargin(0);
    blockFormat.setBottomMargin(0);
    blockFormat.setLeftMargin(0);
    blockFormat.setRightMargin(0);
    blockFormat.setLineHeight(0,QTextBlockFormat::MinimumHeight);


    // 应用格式
    charFormat.setFont(font);               // 应用字体格式
    cursor.setCharFormat(charFormat);       // 应用字符格式
    cursor.setBlockFormat(blockFormat);     // 应用文字块样式

//    qDebug() <<"Set Format";

    cursor.insertText(content);


    // 插入到场景中
    page->addBlock(block,layer);

}

void OFD_DocConvertor::insertCT_Path(DocPage *page, CT_Layer *layer, CT_Path *path)
{

}

void OFD_DocConvertor::insertCT_Image(DocPage *page, DocPage::Layer layer, CT_Image *image)
{

}