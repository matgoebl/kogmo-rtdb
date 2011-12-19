#ifndef VIDEOANNOT_RTDBOBJ_H
#define VIDEOANNOT_RTDBOBJ_H

#ifdef __cplusplus
 #include "kogmo_rtdb.hxx"
 namespace KogniMobil {
 extern "C" {
#endif
#include "kogmo_rtdb.h"



/*! \brief Annotationen zu einem Videobild
 */
typedef struct
{
 uint16_t x1;
 uint16_t y1;
 uint16_t x2;
 uint16_t y2;
 uint8_t color;
 uint8_t width;
 uint16_t flags;
} annotline_t;
#define A2_IMAGEANNOT_MAXLINES 1024

typedef struct
{
  uint16_t numlines;    //!< Anzahl gueltiger Werte in line[]
  uint16_t flags;       //!< irgendwas
  annotline_t line[A2_IMAGEANNOT_MAXLINES];
} kogmo_rtdb_subobj_a2_imageannot_t;

/*! \brief Vollstaendiges Objekt fuer eine Videobild-Annotation
 * Das Annotationsobjekt sollte das betreffende Videobild als
 * als Parent-Objekt haben!
 */
typedef struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_a2_imageannot_t annot;
} kogmo_rtdb_obj_a2_imageannot_t; 
#define _KOGMO_RTDB_OBJTYPE_A2_IMAGEANNOT 0xA20034   // Annotationen (Liniensegmente) zu einem Videobild

// DxEFINE_KOGMO_RTDB_OBJECT( a2_imageannot, 0xA20034, "Annotationen (Liniensegmente) zu einem Videobild" )



/*@}*/

#ifdef __cplusplus
}; /* extern "C" */

typedef RTDBObj_T < kogmo_rtdb_subobj_a2_imageannot_t, _KOGMO_RTDB_OBJTYPE_A2_IMAGEANNOT, RTDBObj > A2_ImageAnnot_T;
class A2_ImageAnnot : public A2_ImageAnnot_T
 {
  public:
    A2_ImageAnnot(class RTDBConn& DBC, const char* name = "") : A2_ImageAnnot_T (DBC, name)
      {
        subobj_p->numlines = 0;
        setMinSize( - (int)sizeof(annotline_t) * A2_IMAGEANNOT_MAXLINES );
      }

    void deleteLines (void)
      {
        subobj_p->numlines = 0;
        calcSize();
      }

    void calcSize (void)
      {
        setSize( - sizeof(annotline_t) * ( A2_IMAGEANNOT_MAXLINES - subobj_p->numlines ) );
      }

    void addPoint ( int x1, int y1,
                   unsigned int color = 1, unsigned int width = 1, unsigned int flags = 0 )
      {
        addLine ( x1, y1, x1, y1, color, width, flags);
      }

    void addLine ( int x1, int y1, int x2 = -1, int y2 = -1,
                   unsigned int color = 1, unsigned int width = 1, unsigned int flags = 0 )
      {
        if ( subobj_p->numlines >= A2_IMAGEANNOT_MAXLINES )
          throw DBError("Too many Annotation Lines, try to increase A2_IMAGEANNOT_MAXLINES");

        annotline_t *a = &(subobj_p->line[subobj_p->numlines]);
        a->x1 = (unsigned int) x1;
        a->y1 = (unsigned int) y1;

        if ( x2==-1 && y2==-1 ) // no x2/y2 given -> draw only a point (x2=x1, y2=y1)
          {
            a->x2 = (unsigned int) x1;
            a->y2 = (unsigned int) y1;
          }
        else
          {
            a->x2 = (unsigned int) x2;
            a->y2 = (unsigned int) y2;
          }
        a->color = color;
        a->width = width;
        a->flags = flags;

        subobj_p->numlines++;
        calcSize();
      };

    int getNumLines (void) const
      {
        return subobj_p->numlines;
      }

    // diese Methode ist kein schoenes C++, man sollte ein annotline-objekt definieren..
    void getLine (const int& nr, annotline_t *a) const
      {
        if ( nr > getNumLines() )
          throw DBError("A annotation line with this number does not exist");
        *a = subobj_p->line[nr];
      }

    std::string dump(void) const
    {
        std::ostringstream ostr;
        ostr << "* Annotations :" << std::endl
             << "flags: " << subobj_p->flags << std::endl;
        for (int i = 0; i < subobj_p->numlines; i++ )
          {
             ostr << i+1 << ".: "
                  << subobj_p->line[i].x1 << "," << subobj_p->line[i].y1 << "-"
                  << subobj_p->line[i].x2 << "," << subobj_p->line[i].y2
                  << " color:" << (int)subobj_p->line[i].color << std::endl;
          }
        return RTDBObj::dump() + ostr.str();
      }; 
};

}; /* namespace KogniMobil */

#endif

#endif
