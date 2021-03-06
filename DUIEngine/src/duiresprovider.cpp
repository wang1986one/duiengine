//////////////////////////////////////////////////////////////////////////
//   File Name: duiresprovider.cpp
// Description: Resource Provider
//////////////////////////////////////////////////////////////////////////
#include "duistd.h"
#include "duiresprovider.h"
#include "mybuffer.h"
#include <io.h>

namespace DuiEngine
{

DuiResProviderPE::DuiResProviderPE( HINSTANCE hInst ,CDuiImgDecoder *pImgDecoder)
    : DuiResProviderBase(pImgDecoder),m_hResInst(hInst)
{

}

HBITMAP DuiResProviderPE::LoadBitmap( LPCTSTR strType,UINT uID )
{
	return ::LoadBitmap(m_hResInst,MAKEINTRESOURCE(uID));
}

HICON DuiResProviderPE::LoadIcon( LPCTSTR strType,UINT uID ,int cx/*=0*/,int cy/*=0*/)
{
    return (HICON)::LoadImage(m_hResInst, MAKEINTRESOURCE(uID), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
}

CDuiImgBase * DuiResProviderPE::LoadImage( LPCTSTR strType,UINT uID )
{
    if(!HasResource(strType,uID)) return NULL;
    CDuiImgBase *pImg=GetImageDecoder()->CreateDuiImage(strType);
    if(pImg)
    {
        if(!pImg->LoadFromResource(m_hResInst,strType,uID))
        {
            GetImageDecoder()->DestoryDuiImage(pImg);
            pImg=NULL;
        }
    }
    return pImg;
}

size_t DuiResProviderPE::GetRawBufferSize( LPCTSTR strType,UINT uID )
{
    HRSRC hRsrc = MyFindResource(strType,uID);

    if (NULL == hRsrc)
        return 0;

    return ::SizeofResource(m_hResInst, hRsrc);
}

BOOL DuiResProviderPE::GetRawBuffer( LPCTSTR strType,UINT uID,LPVOID pBuf,size_t size )
{
    DUIASSERT(strType);
    HRSRC hRsrc = MyFindResource(strType,uID);

    if (NULL == hRsrc)
        return FALSE;

    size_t dwSize = ::SizeofResource(m_hResInst, hRsrc);
    if (0 == dwSize)
        return FALSE;

    if(size < dwSize)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    HGLOBAL hGlobal = ::LoadResource(m_hResInst, hRsrc);
    if (NULL == hGlobal)
        return FALSE;

    LPVOID pBuffer = ::LockResource(hGlobal);
    if (NULL == pBuffer)
        return FALSE;

    memcpy(pBuf,pBuffer,dwSize);

    ::FreeResource(hGlobal);

    return TRUE;
}

BOOL DuiResProviderPE::HasResource( LPCTSTR strType,UINT uID )
{
    DUIASSERT(strType);
    return MyFindResource(strType,uID)!=NULL;
}

HRSRC DuiResProviderPE::MyFindResource( LPCTSTR strType, UINT uID )
{
    if(_tcsicmp(strType,DUIRES_BMP_TYPE)==0) strType=MAKEINTRESOURCE(2);//RT_BITMAP;
    else if(_tcsicmp(strType,DUIRES_ICON_TYPE)==0) strType=MAKEINTRESOURCE(3);//RT_ICON;

    return ::FindResource(m_hResInst, MAKEINTRESOURCE(uID), strType);
}


DuiResProviderFiles::DuiResProviderFiles(CDuiImgDecoder *pImgDecoder):DuiResProviderBase(pImgDecoder)
{
}

CDuiStringT DuiResProviderFiles::GetRes( LPCTSTR strType,UINT uID )
{
    DuiResID resID(strType,uID);
    CDuiMap<DuiResID,CDuiStringT>::CPair *p=m_mapFiles.Lookup(resID);
    if(!p) return _T("");
    CDuiStringT strRet=m_strPath+_T("\\")+p->m_value;
    return strRet;
}

HBITMAP DuiResProviderFiles::LoadBitmap( LPCTSTR strType,UINT uID )
{
    CDuiStringT strPath=GetRes(strType,uID);
    if(strPath.IsEmpty()) return NULL;
    return (HBITMAP)::LoadImage(NULL, strPath, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
}

HICON DuiResProviderFiles::LoadIcon( LPCTSTR strType,UINT uID ,int cx/*=0*/,int cy/*=0*/)
{
    CDuiStringT strPath=GetRes(strType,uID);
    if(strPath.IsEmpty()) return NULL;
    return (HICON)::LoadImage(NULL, strPath, IMAGE_ICON, cx, cy, LR_LOADFROMFILE);
}

CDuiImgBase * DuiResProviderFiles::LoadImage( LPCTSTR strType,UINT uID )
{
    if(!HasResource(strType,uID)) return NULL;
    CDuiImgBase * pImg=GetImageDecoder()->CreateDuiImage(strType);
    if(pImg)
    {
        CDuiStringT strPath=GetRes(strType,uID);
        if(!pImg->LoadFromFile(strPath))
        {
            GetImageDecoder()->DestoryDuiImage(pImg);
            pImg=NULL;
        }
    }
    return pImg;
}

size_t DuiResProviderFiles::GetRawBufferSize( LPCTSTR strType,UINT uID )
{
    CDuiStringT strPath=GetRes(strType,uID);
    if(strPath.IsEmpty()) return 0;
    WIN32_FIND_DATA wfd;
    HANDLE hf=FindFirstFile(strPath,&wfd);
    if(INVALID_HANDLE_VALUE==hf) return 0;
    FindClose(hf);
    return wfd.nFileSizeLow;
}

BOOL DuiResProviderFiles::GetRawBuffer( LPCTSTR strType,UINT uID,LPVOID pBuf,size_t size )
{
    CDuiStringT strPath=GetRes(strType,uID);
    if(strPath.IsEmpty()) return FALSE;
    FILE *f=_tfopen(strPath,_T("rb"));
    if(!f) return FALSE;
    size_t len=_filelength(_fileno(f));
    if(len>size)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        fclose(f);
        return FALSE;
    }
    BOOL bRet=(len==fread(pBuf,1,len,f));

    fclose(f);
    return bRet;
}

BOOL DuiResProviderFiles::Init( LPCTSTR pszPath )
{
    CMyBuffer<char>  xmlBuf;
    CDuiStringT strPathIndex=pszPath;
    strPathIndex+=_T("\\");
    strPathIndex+=INDEX_XML;
    FILE *f=_tfopen(strPathIndex,_T("rb"));
    if(!f) return(FALSE);
    int nLen=_filelength(_fileno(f));
    if(nLen>100*1024)
    {
        fclose(f);
        return FALSE;
    }
    xmlBuf.Allocate(nLen);
    if(nLen!=fread(xmlBuf,1,nLen,f))
    {
        fclose(f);
        return FALSE;
    }
    fclose(f);

	pugi::xml_document xmlDoc;
    CDuiStringT strFileName;
	if(!xmlDoc.load_buffer(xmlBuf,xmlBuf.size(),pugi::parse_default,pugi::encoding_utf8)) return FALSE;

	pugi::xml_node xmlNode=xmlDoc.child("resid");
    while(xmlNode)
    {
		DuiResID id(DUI_CA2T(xmlNode.attribute("type").value(),CP_UTF8),xmlNode.attribute("id").as_int(0));
		CDuiStringT strFile=DUI_CA2T(xmlNode.attribute("file").value(),CP_UTF8);
		if(!m_strPath.IsEmpty()) strFile.Format(_T("%s\\%s"),(LPCTSTR)m_strPath,(LPCTSTR)strFile);
		m_mapFiles[id]=strFile;
		xmlNode=xmlNode.next_sibling("resid");
    }

    m_strPath=pszPath;
    return TRUE;
}

BOOL DuiResProviderFiles::HasResource( LPCTSTR strType,UINT uID )
{
    DuiResID resID(strType,uID);
    CDuiMap<DuiResID,CDuiStringT>::CPair *p=m_mapFiles.Lookup(resID);
    return (p!=NULL);
}

}//namespace DuiEngine