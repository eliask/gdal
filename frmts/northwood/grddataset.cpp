/******************************************************************************
 * $Id$
 *
 * Project:  GRD Reader
 * Purpose:  GDAL driver for Northwood Grid Format
 * Author:   Perry Casson
 *
 ******************************************************************************
 * Copyright (c) 2006, Waypoint Information Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "gdal_pam.h"
#include "northwood.h"

#ifdef MSVC
#include "..\..\ogr\ogrsf_frmts\mitab\mitab.h"
#else
#include "../../ogr/ogrsf_frmts/mitab/mitab.h"
#endif


CPL_C_START void GDALRegister_NWT_GRD( void );
CPL_C_END
/************************************************************************/
/* ==================================================================== */
/*                      NWT_GRDDataset                                  */
/* ==================================================================== */
/************************************************************************/
class NWT_GRDRasterBand;

class NWT_GRDDataset:public GDALPamDataset
{
  friend class NWT_GRDRasterBand;

    FILE *fp;
    GByte abyHeader[1024];
    NWT_GRID *pGrd;
    NWT_RGB ColorMap[4096];
    char *pszProjection;

  public:
    NWT_GRDDataset();
    ~NWT_GRDDataset();

    static GDALDataset *Open( GDALOpenInfo * );

    CPLErr GetGeoTransform( double *padfTransform );
    const char *GetProjectionRef();
};

/************************************************************************/
/* ==================================================================== */
/*                            NWT_GRDRasterBand                         */
/* ==================================================================== */
/************************************************************************/

class NWT_GRDRasterBand:public GDALPamRasterBand
{
  friend class NWT_GRDDataset;

    int bHaveOffsetScale;
    double dfOffset;
    double dfScale;

  public:

    NWT_GRDRasterBand( NWT_GRDDataset *, int );

    virtual CPLErr IReadBlock( int, int, void * );
    virtual double GetNoDataValue( int *pbSuccess );

    virtual double GetOffset( int *pbSuccess = NULL );
    virtual CPLErr SetOffset( double dfNewValue );
    virtual double GetScale( int *pbSuccess = NULL );
    virtual CPLErr SetScale( double dfNewValue );

    virtual GDALColorInterp GetColorInterpretation();
};


/************************************************************************/
/*                           NWT_GRDRasterBand()                        */
/************************************************************************/
NWT_GRDRasterBand::NWT_GRDRasterBand( NWT_GRDDataset * poDS, int nBand )
{
    this->poDS = poDS;
    this->nBand = nBand;

    if( nBand == 4 )
    {
        bHaveOffsetScale = TRUE;
        dfOffset = poDS->pGrd->fZMin;

        if( poDS->pGrd->cFormat == 0x01 )
        {
            eDataType = GDT_Float32;
            dfScale =( poDS->pGrd->fZMax - poDS->pGrd->fZMin ) / 4294967294.0;
        }
        else
        {
            eDataType = GDT_Float32;
            dfScale =( poDS->pGrd->fZMax - poDS->pGrd->fZMin ) / 65534.0;
        }
    }
    else
    {
        bHaveOffsetScale = FALSE;
        dfOffset = 0;
        dfScale = 1.0;
        eDataType = GDT_Byte;
    }
    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;
}

double NWT_GRDRasterBand::GetNoDataValue( int *pbSuccess )
{
    if( pbSuccess != NULL )
        *pbSuccess = TRUE;

    return 0;                    //Northwood grid 0 is always null
}

GDALColorInterp NWT_GRDRasterBand::GetColorInterpretation()
{
    //return GCI_RGB;
    if( nBand == 4 )
        return GCI_Undefined;
    else if( nBand == 1 )
        return GCI_RedBand;
    else if( nBand == 2 )
        return GCI_GreenBand;
    else if( nBand == 3 )
        return GCI_BlueBand;
    else
        return GCI_Undefined;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/
CPLErr NWT_GRDRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff, void *pImage )
{
    NWT_GRDDataset *poGDS = (NWT_GRDDataset *) poDS;
    char *pszRecord;
    int nRecordSize = nBlockXSize * 2;
    int i;
    unsigned short raw1;

    VSIFSeek( poGDS->fp, 1024 + nRecordSize * nBlockYOff, SEEK_SET );

    pszRecord = (char *) CPLMalloc( nRecordSize );
    VSIFRead( pszRecord, 1, nRecordSize, poGDS->fp );

    if( nBand == 4 )                //Z values
    {
        for( i = 0; i < nBlockXSize; i++ )
        {
            memcpy( (void *) &raw1, (void *)(pszRecord + 2 * i), 2 );
            if( raw1 == 0 )
            {
                ((float *)pImage)[i] = -1.e37;    // null value
            }
            else
            {
                ((float *)pImage)[i] = dfOffset + ((raw1 - 1) * dfScale);
            }
        }
    }
    else if( nBand == 1 )            // red values
    {
        for( i = 0; i < nBlockXSize; i++ )
        {
            memcpy( (void *) &raw1, (void *)(pszRecord + 2 * i), 2 );
            ((char *)pImage)[i] = poGDS->ColorMap[raw1 / 16].r;
        }
    }
    else if( nBand == 2 )            // green
    {
        for( i = 0; i < nBlockXSize; i++ )
        {
            memcpy( (void *) &raw1, (void *)(pszRecord + 2 * i), 2 );
            ((char *) pImage)[i] = poGDS->ColorMap[raw1 / 16].g;
        }
    }
    else if( nBand == 3 )            // blue
    {
        for( i = 0; i < nBlockXSize; i++ )
        {
            memcpy( (void *) &raw1, (void *) (pszRecord + 2 * i), 2 );
            ((char *) pImage)[i] = poGDS->ColorMap[raw1 / 16].b;
        }
    }
    else
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "No band number %d",
                  nBand );
        if( pszRecord != NULL )
            CPLFree( pszRecord );
        return CE_Failure;
    }
    if( pszRecord != NULL )
    {
        CPLFree( pszRecord );
    }
    return CE_None;
}

/************************************************************************/
/*                             GetOffset()                              */
/************************************************************************/
double NWT_GRDRasterBand::GetOffset( int *pbSuccess )
{
    if( pbSuccess )
        *pbSuccess = bHaveOffsetScale;
    return dfOffset;
}

/************************************************************************/
/*                             SetOffset()                              */
/************************************************************************/
CPLErr NWT_GRDRasterBand::SetOffset( double dfNewValue )
{
    //poGDS->bMetadataChanged = TRUE;

    bHaveOffsetScale = TRUE;
    dfOffset = dfNewValue;
    return CE_None;
}

/************************************************************************/
/*                              GetScale()                              */
/************************************************************************/
double NWT_GRDRasterBand::GetScale( int *pbSuccess )
{
    if( pbSuccess )
        *pbSuccess = bHaveOffsetScale;
    return dfScale;
}

/************************************************************************/
/*                              SetScale()                              */
/************************************************************************/
CPLErr NWT_GRDRasterBand::SetScale( double dfNewValue )
{
    bHaveOffsetScale = TRUE;
    dfScale = dfNewValue;
    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                             NWT_GRDDataset                           */
/* ==================================================================== */
/************************************************************************/
NWT_GRDDataset::NWT_GRDDataset()
{
    pszProjection = NULL;
    //poCT = NULL;
}


/************************************************************************/
/*                            ~NWT_GRDDataset()                         */
/************************************************************************/

NWT_GRDDataset::~NWT_GRDDataset()
{
    FlushCache();
    pGrd->fp = NULL;       // this prevents nwtCloseGrid from closing the fp 
    nwtCloseGrid( pGrd );

    if( fp != NULL )
        VSIFClose( fp );

    if( pszProjection != NULL )
    {
        CPLFree( pszProjection );
    }
    /*if( poCT != NULL )
        delete poCT;*/
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/
CPLErr NWT_GRDDataset::GetGeoTransform( double *padfTransform )
{
    padfTransform[0] = pGrd->dfMinX - ( pGrd->dfStepSize * 0.5 );
    padfTransform[3] = pGrd->dfMaxY + ( pGrd->dfStepSize * 0.5 );
    padfTransform[1] = pGrd->dfStepSize;
    padfTransform[2] = 0.0;

    padfTransform[4] = 0.0;
    padfTransform[5] = -1 * pGrd->dfStepSize;

    return CE_None;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/
const char *NWT_GRDDataset::GetProjectionRef()
{
    OGRSpatialReference *poSpatialRef;
    poSpatialRef = MITABCoordSys2SpatialRef( pGrd->cMICoordSys );
    poSpatialRef->exportToWkt( &pszProjection );
    poSpatialRef->Release();
    poSpatialRef = NULL;
    return( (const char *)pszProjection );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/
GDALDataset *NWT_GRDDataset::Open( GDALOpenInfo * poOpenInfo )
{
/* -------------------------------------------------------------------- */
/*  Look for the header                                                 */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->fp == NULL || poOpenInfo->nHeaderBytes < 50 )
        return NULL;

    if( poOpenInfo->pabyHeader[0] != 'H' ||
        poOpenInfo->pabyHeader[1] != 'G' ||
        poOpenInfo->pabyHeader[2] != 'P' ||
        poOpenInfo->pabyHeader[3] != 'C' || poOpenInfo->pabyHeader[4] != '1' )
    return NULL;

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    NWT_GRDDataset *poDS;

    poDS = new NWT_GRDDataset();

    poDS->fp = poOpenInfo->fp;
    poOpenInfo->fp = NULL;

/* -------------------------------------------------------------------- */
/*      Read the header.                                                */
/* -------------------------------------------------------------------- */
    VSIFSeek( poDS->fp, 0, SEEK_SET );
    VSIFRead( poDS->abyHeader, 1, 1024, poDS->fp );
    poDS->pGrd = (NWT_GRID *) CPLMalloc(sizeof(NWT_GRID));
    nwt_ParseHeader( poDS->pGrd, (char *)poDS->abyHeader );

    poDS->nRasterXSize = poDS->pGrd->nXSide;
    poDS->nRasterYSize = poDS->pGrd->nYSide;

// create a colorTable
  // if( poDS->pGrd->iNumColorInflections > 0 )
  //   poDS->CreateColorTable();
  nwt_LoadColors( poDS->ColorMap, 4096, poDS->pGrd );
/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    poDS->SetBand( 1, new NWT_GRDRasterBand( poDS, 1 ) );    //r
    poDS->SetBand( 2, new NWT_GRDRasterBand( poDS, 2 ) );    //g
    poDS->SetBand( 3, new NWT_GRDRasterBand( poDS, 3 ) );    //b
    poDS->SetBand( 4, new NWT_GRDRasterBand( poDS, 4 ) );    //z

/* -------------------------------------------------------------------- */
/*      Initialize any PAM information.                                 */
/* -------------------------------------------------------------------- */
    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->TryLoadXML();

    return (poDS);
}


/************************************************************************/
/*                          GDALRegister_GRD()                          */
/************************************************************************/
void GDALRegister_NWT_GRD()
{
    GDALDriver *poDriver;

    if( GDALGetDriverByName( "NWT_GRD" ) == NULL )
    {
        poDriver = new GDALDriver();

        poDriver->SetDescription( "Northwood_GRD" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME,
                                 "Northwood Numeric Grid Format .grd/.tab" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, "frmt_various.html#grd");
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "grd" );

        poDriver->pfnOpen = NWT_GRDDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}
