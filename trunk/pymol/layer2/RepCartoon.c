/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* copyright 1998-2000 by Warren Lyford Delano of DeLano Scientific. 
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/

#include"os_std.h"
#include"os_gl.h"

#include"Base.h"
#include"OOMac.h"
#include"RepCartoon.h"
#include"Color.h"
#include"Setting.h"
#include"Word.h"
#include"Scene.h"
#include"main.h"
#include"Feedback.h"
#include"CGO.h"
#include"Extrude.h"

typedef struct RepCartoon {
  Rep R; /* must be first! */
  ObjectMolecule *obj;
  CoordSet *cs;
  CGO *ray,*std;
} RepCartoon;

#include"ObjectMolecule.h"

void RepCartoonRender(RepCartoon *I,CRay *ray,Pickable **pick);
void RepCartoonFree(RepCartoon *I);

void RepCartoonFree(RepCartoon *I)
{
  if(I->ray)
    CGOFree(I->ray);
  if(I->std)
    CGOFree(I->std);
  RepFree(&I->R);
  OOFreeP(I);
}

void RepCartoonRender(RepCartoon *I,CRay *ray,Pickable **pick)
{
  if(ray) {
    PRINTFD(FB_RepCartoon)
      " RepCartoonRender: rendering raytracable...\n"
      ENDFD;
    
    if(I->ray)  
      CGORenderRay(I->ray,ray,NULL,I->cs->Setting,
                   I->obj->Obj.Setting
                   );
    else if(I->std)
      CGORenderRay(I->std,ray,NULL,I->cs->Setting,
                   I->obj->Obj.Setting);    
  } else if(pick&&PMGUI) {
  } else if(PMGUI) {
    
    PRINTFD(FB_RepCartoon)
      " RepCartoonRender: rendering GL...\n"
      ENDFD;

    if(I->std) 
      CGORenderGL(I->std,NULL,I->cs->Setting,
                   I->obj->Obj.Setting);
  }
}

static float smooth(float x,float power)
{

  if(x<=0.5) {
    if(x<=0.0) x=0.0;
    return (0.5*pow(2.0*x,power));    
  } else {
    if(x>=1.0) x=1.0;
    return (1.0-(0.5*pow(2*(1.0-x),power)));
  }
}

Rep *RepCartoonNew(CoordSet *cs)
{
  ObjectMolecule *obj;
  int a,b,c,f,e,a1,a2,c1,c2,*i,*s,*at,*seg,nAt,*atp,a3,a4=0,*car,*cc,*sstype;
  float *v,*v0,*v1,*v2,*v3,*v4,*v5,*vo,*vn,*va;
  float *p0,*p1,*p2;
  float *pv=NULL;
  float *pvo=NULL,*pva=NULL;
  float *dv=NULL;
  float *nv=NULL;
  float *tv=NULL;
  float *vc=NULL;
  float *tmp=NULL;
  int last,first,end_flag;
  float f0,f1,f2,f3,f4,dev;
  float *d,dp;
  float *dl=NULL;
  int nSeg;
  int sampling;
  int *ss;
  float  power_a = 5;
  float power_b = 5;
  float loop_radius;
  float tube_radius,tube_quality;
  int visFlag;
  CExtrude *ex,*ex1;
  int n_p;
  int loop_quality;
  float oval_quality,oval_width,oval_length;
  float dumbbell_radius,dumbbell_width,dumbbell_length;
  float throw;
  int st,nd;
  float *v_c,*v_n,*v_o,*v_ca;
  float t0[3],t1[3],t2[3],o0[12],o1[12];
  float max_dot;
  float length,width;
  int cur_car;
  int contFlag,extrudeFlag;
  int cartoon_debug;
  int fancy_helices;
  int fancy_sheets;
  int refine;
  int contigFlag;
  int discrete_colors;

  float refine_tips;

  OOAlloc(RepCartoon);

PRINTFD(FB_RepCartoon)
" RepCartoonNew-Debug: entered.\n"
ENDFD;
	

  obj = cs->Obj;
  visFlag=false;
  for(a=0;a<cs->NIndex;a++) {
	 if(obj->AtomInfo[cs->IdxToAtm[a]].visRep[cRepCartoon])
		{
		  visFlag=true;
		  break;
		}
  }
  if(!visFlag) {
    OOFreeP(I);
    return(NULL); /* skip if no dots are visible */
  }

  RepInit(&I->R);
  power_a=SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_power);
  power_b=SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_power_b);

  cartoon_debug=SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_debug);
  length=SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_rect_length);
  width=SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_rect_width);

  throw=SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_throw);

  sampling = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_sampling);
  if(sampling<1) sampling=1;
  loop_radius = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_loop_radius);
  if(loop_radius<0.01) loop_radius=0.01;
  loop_quality = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_loop_quality);
  if(loop_quality<3) loop_quality=3;

  tube_radius = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_tube_radius);
  if(tube_radius<0.01) tube_radius=0.01;
  tube_quality = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_tube_quality);
  if(tube_quality<3) tube_quality=3;

  oval_length = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_oval_length);
  oval_width = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_oval_width);
  oval_quality = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_oval_quality);
  if(oval_quality<3) tube_quality=3;

  dumbbell_length = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_dumbbell_length);
  dumbbell_width = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_dumbbell_width);
  dumbbell_radius = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_dumbbell_radius);
  if(dumbbell_radius<0.01) dumbbell_radius=0.01;

  fancy_helices = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_fancy_helices);
  fancy_sheets = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_fancy_sheets);
  refine = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_refine);
  refine_tips = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_refine_tips);

  discrete_colors = SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_discrete_colors);

  I->R.fRender=(void (*)(struct Rep *, CRay *, Pickable **))RepCartoonRender;
  I->R.fFree=(void (*)(struct Rep *))RepCartoonFree;
  I->R.fRecolor=NULL;
  I->obj=obj;
  I->cs=cs;
  I->ray=NULL;
  I->std=NULL;

  /* find all of the CA points */

  at = Alloc(int,cs->NAtIndex);
  pv = Alloc(float,cs->NAtIndex*3);
  tmp = Alloc(float,cs->NAtIndex*3);
  pvo = Alloc(float,cs->NAtIndex*3); /* orientation vector */
  pva = Alloc(float,cs->NAtIndex*6); /* alternative orientation vectors */
  seg = Alloc(int,cs->NAtIndex);
  car = Alloc(int,cs->NAtIndex);
  sstype = Alloc(int,cs->NAtIndex);

  i=at;
  v=pv;
  vo=pvo;
  s=seg;
  cc=car;
  ss=sstype;
  nAt = 0;
  nSeg = 0;
  a2=-1;
  for(a1=0;a1<cs->NAtIndex;a1++)
	 {
      if(obj->DiscreteFlag) {
        if(cs==obj->DiscreteCSet[a1]) 
          a=obj->DiscreteAtmToIdx[a1];
        else 
          a=-1;
      } else 
        a=cs->AtmToIdx[a1];
		if(a>=0)
		  if(obj->AtomInfo[a1].visRep[cRepCartoon])
			 if(!obj->AtomInfo[a1].hetatm)
            if((!obj->AtomInfo[a1].alt[0])||
               (obj->AtomInfo[a1].alt[0]=='A')) {
              if(WordMatch("CA",obj->AtomInfo[a1].name,1)<0)
                {
                  if(a2>=0) {
                    /*
                      if((abs(obj->AtomInfo[a1].resv-obj->AtomInfo[a2].resv)>1)||
                      (obj->AtomInfo[a1].chain[0]!=obj->AtomInfo[a2].chain[0])||
                      (!WordMatch(obj->AtomInfo[a1].segi,obj->AtomInfo[a2].segi,1)))*/
                      if(!ObjectMoleculeCheckBondSep(obj,a1,a2,3)) /* CA->N->C->CA = 3 bonds */
                        a2=-1;

                  }
                  if(a2<=0)
                    nSeg++;
                  *(s++) = nSeg;
                  nAt++;
                  *(i++)=a;
                  cur_car = obj->AtomInfo[a1].cartoon;
                  switch (obj->AtomInfo[a1].ssType[0]) {
                  case 'H':
                  case 'h':
                    if (cur_car==cCartoon_auto) {
                      if(fancy_helices)
                        cur_car = cCartoon_dumbbell;
                      else
                        cur_car = cCartoon_oval;
                    }
                    *ss=1; /* helix */
                    break;
                  case 'S':
                  case 's':
                    if (cur_car==cCartoon_auto) {
                      if(fancy_sheets)
                        cur_car = cCartoon_arrow;
                      else
                        cur_car = cCartoon_rect;
                    }
                    *ss=2;
                    break;
                  default: /* 'L', 'T', 0, etc. */
                    if (cur_car==cCartoon_auto) {
                      cur_car = cCartoon_loop;
                    }
                    *ss=0;
                    break;
                  }
                  *(cc++)=cur_car;
                  v1 = cs->Coord+3*a;
                  v_ca = v1;
                  *(v++)=*(v1++);
                  *(v++)=*(v1++);
                  *(v++)=*(v1++);
                  a2=a1;
                  
                  ss++;

                  v_c = NULL;
                  v_n = NULL;
                  v_o = NULL;
                  
                  AtomInfoBracketResidueFast(obj->AtomInfo,obj->NAtom,a1,&st,&nd);
                  
                  for(a3=st;a3<=nd;a3++) {
                    
                    if(obj->DiscreteFlag) {
                      if(cs==obj->DiscreteCSet[a3]) 
                        a4=obj->DiscreteAtmToIdx[a3];
                      else 
                        a4=-1;
                    } else 
                      a4=cs->AtmToIdx[a3];
                    if(a4>=0) {
                      if(WordMatch("C",obj->AtomInfo[a3].name,1)<0) {
                        v_c = cs->Coord+3*a4;		
                      } else if(WordMatch("N",obj->AtomInfo[a3].name,1)<0) {
                        v_n = cs->Coord+3*a4;
                      } else if(WordMatch("O",obj->AtomInfo[a3].name,1)<0) {
                        v_o = cs->Coord+3*a4;
                      }
                    }
                  }
                  if(!(v_c&&v_n&&v_o)) {
                    vo[0]=0.0;
                    vo[1]=0.0;
                    vo[2]=0.0;
                    vo+=3;
                  } else {
                    /* generate orientation vectors...*/

                    subtract3f(v_n,v_c,t0);
                    normalize3f(t0);
                    subtract3f(v_n,v_o,t1);
                    normalize3f(t1);
                    cross_product3f(t0,t1,vo);
                    normalize3f(vo);
                    
                    vo+=3;
                  }
                } else if(WordMatch("P",obj->AtomInfo[a1].name,1)<0) {
                  if(a2>=0) {
                    if(!ObjectMoleculeCheckBondSep(obj,a1,a2,6)) /* six bonds between phosphates */
                      a2=-1;
                  }
                  if(a2<=0)
                    nSeg++;
                  *(s++) = nSeg;
                  nAt++;
                  *(i++)=a;
                  cur_car = obj->AtomInfo[a1].cartoon;
                  if(cur_car == cCartoon_auto)
                    cur_car = cCartoon_tube;
                  *ss=3; /* DNA/RNA */

                  *(cc++)=cur_car;
                  v1 = cs->Coord+3*a;
                  v_ca = v1;
                  *(v++)=*(v1++);
                  *(v++)=*(v1++);
                  *(v++)=*(v1++);
                  a2=a1;
                  
                  ss++;

                  v_c = NULL;
                  v_n = NULL;
                  v_o = NULL;
                  
                  AtomInfoBracketResidueFast(obj->AtomInfo,obj->NAtom,a1,&st,&nd);
                  
                  for(a3=st;a3<=nd;a3++) {
                    
                    if(obj->DiscreteFlag) {
                      if(cs==obj->DiscreteCSet[a3]) 
                        a4=obj->DiscreteAtmToIdx[a3];
                      else 
                        a4=-1;
                    } else 
                      a4=cs->AtmToIdx[a3];
                    if(a4>=0) {
                      
                      /* need to figure out how to generate a how to do right for DNA...
                         then we can use oval or rectangle */
                      
                      if(WordMatch("P",obj->AtomInfo[a3].name,1)<0) {
                        v_c = cs->Coord+3*a4;		
                      } else if(WordMatch("C2",obj->AtomInfo[a3].name,1)<0) {
                        v_o = cs->Coord+3*a4;
                      }
                    }
                  }
                  if(!(v_c&&v_o)) {
                    vo[0]=0.0;
                    vo[1]=0.0;
                    vo[2]=0.0;
                    vo+=3;
                  } else {
                    /* generate orientation vectors...*/
                    
                    cross_product3f(v_c,v_o,vo);
                    normalize3f(vo);
                    
                    vo+=3;
                  }
                }
             
            }

	 }

PRINTFD(FB_RepCartoon)
" RepCartoon-Debug: path outlined, interpolating...\n"
ENDFD;

  if(nAt)
	 {
		/* compute differences and normals */

		s=seg;
		v=pv;
		
		dv = Alloc(float,nAt*3);
		nv = Alloc(float,nAt*3);
		dl = Alloc(float,nAt);
		v1=dv;
		v2=nv;
		d=dl;
		for(a=0;a<(nAt-1);a++)
		  {
			 if(*s==*(s+1))
				{
				  subtract3f(v+3,v,v1);
				  *d = length3f(v1);
              if(*d>R_SMALL4) {
                scale3f(v1,1.0/(*d),v2);
              } else if(a)  {
                copy3f(v2-3,v2); 
              } else {
                zero3f(v2);
              }
				}
          else {
            zero3f(v2);	
          }
          
			 d++;
			 v+=3;
			 v1+=3;
			 v2+=3;
			 s++;
		  }
		
		/* compute tangents */
		
		s=seg;
		v=nv;
		
		tv = Alloc(float,nAt*3+6);
		v1=tv;
		
		*(v1++)=*(v++); /* first segment */
		*(v1++)=*(v++);
		*(v1++)=*(v++);
		s++;
		
		for(a=1;a<(nAt-1);a++)
		  {
			 if((*s==*(s-1))&&(*s==*(s+1)))
				{
				  add3f(v,(v-3),v1);
				  normalize3f(v1);			 
				}
			 else if(*s==*(s-1))
				{
				  *(v1)=*(v-3);  /* end a segment */
				  *(v1+1)=*(v-2); 
				  *(v1+2)=*(v-1); 
				}
			 else if(*s==*(s+1))
				{
				  *(v1)=*(v);   /* new segment */
				  *(v1+1)=*(v+1); 
				  *(v1+2)=*(v+2); 
				}
			 v+=3;
			 v1+=3;
			 s++;
		  }
		
		*(v1++)=*(v-3); /* last segment */
		*(v1++)=*(v-2);
		*(v1++)=*(v-1);

      PRINTFD(FB_RepCartoon)
        " RepCartoon-Debug: generating coordinate systems...\n"
        ENDFD;
      
      if(SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_round_helices)) {
        v1 = NULL;
        v2 = NULL;
        v3 = NULL;
        v4 = NULL;
        v5 = NULL;
        s = seg;
        v = pv;
        ss = sstype;
        vo = pvo;
        v0 = tv;
        last = 0;
        if(nAt>1) {
          for(a=0;a<nAt;a++) {
            if(a) {
              if(*s!=*(s-1)) { /* contiguous helices in disconnected segments */
                v1 = NULL;
                v2 = NULL;
                v3 = NULL;
                v4 = NULL;
                v5 = NULL;
                last = 0;
              }
            }
            v5 = v4;
            v4 = v3;
            v3 = v2;
            v2 = v1;
            if(*ss==1)
              v1 = v;
            else {
              if(last<2) {/* not 5+ turn helix, so just average the tangents */
                /*                if(v2&&v3) {
                                  copy3f(v0-3,t0);
                                  copy3f(v0-6,t1);
                                  if(dot_product3f(t0,t1)<0.0)
                                  invert3f(t1);
                                  add3f(t1,t0,t0);
                                  if(v4) {
                                  copy3f(v0-9,t1);
                                  if(dot_product3f(t0,t1)<0.0)
                                  invert3f(t1);
                                  add3f(t1,t0,t0);
                                  }
                                  if(v5) {
                                  copy3f(v0-12,t1);
                                  if(dot_product3f(t0,t1)<0.0)
                                  invert3f(t1);
                                  add3f(t1,t0,t0);
                                  }
                                  normalize3f(t0);
                                  cross_product3f(t0,v0-3,vo-3);
                                  normalize3f(vo-3);
                                  cross_product3f(t0,v0-6,vo-6);
                                  normalize3f(vo-6);
                                  if(v4) {
                                  cross_product3f(t0,v0-9,vo-9);
                                  normalize3f(vo-9);
                                  }
                                  if(v5) {
                                  cross_product3f(t0,v0-12,vo-12);
                                  normalize3f(vo-12);
                                  }
                */
                zero3f(t0);
                if(v2&&v3) {
                  subtract3f(v2,v,t0);
                  normalize3f(t0);
                  subtract3f(v3,v2,t1);
                  normalize3f(t1);
                  add3f(t1,t0,t0);
                  if(v4) {
                    subtract3f(v4,v3,t1);
                    normalize3f(t1);
                    add3f(t1,t0,t0);
                  }
                  if(v5) {
                    subtract3f(v5,v4,t1);
                    normalize3f(t1);
                    add3f(t1,t0,t0);
                  }
                  normalize3f(t0);
                  cross_product3f(t0,v0-3,vo-3);
                  normalize3f(vo-3);
                  cross_product3f(t0,v0-6,vo-6);
                  normalize3f(vo-6);
                  if(v4) {
                    cross_product3f(t0,v0-9,vo-9);
                    normalize3f(vo-9);
                  }
                  if(v5) {
                    cross_product3f(t0,v0-12,vo-12);
                    normalize3f(vo-12);
                  }
                }
              }
              v1 = NULL;
              v2 = NULL;
              v3 = NULL;
              v4 = NULL;
              v5 = NULL;
              last = 0;
            }
            if(v1&&v2&&v3&&v4) {
              add3f(v1,v4,t0);
              add3f(v2,v3,t1);
              scale3f(t0,0.2024,t0);
              scale3f(t1,0.2727,t1);
              add3f(t0,t1,t0);
              if(last) { /* 5th CA or later... */
                subtract3f(t2,t0,t1);
                normalize3f(t1);
                cross_product3f(t1,v0,vo);
                normalize3f(vo);
                cross_product3f(t1,v0-3,vo-3);
                normalize3f(vo-3);
                cross_product3f(t1,v0-6,vo-6);
                normalize3f(vo-6);
                if(last==1) { /* 5th */
                  cross_product3f(t1,v0-9,vo-9);
                  normalize3f(vo-9);
                  cross_product3f(t1,v0-12,vo-12);
                  normalize3f(vo-12);
                }
              }
              last++;
              copy3f(t0,t2);
            }
            v+=3;
            ss++;
            vo+=3;
            v0+=3;
            s++;
          }
        }
      }

      if(SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_refine_normals)) {
        /* now generate alternative orientation vectors */
        
        v1 = tv;
        va = pva;
        vo = pvo;
        ss = sstype;
        for(a=0;a<nAt;a++) { 
          
          /* original */
          copy3f(vo,va);
          va+=3;
          
          /* inverse */
          copy3f(vo,va);
          if(*ss!=1)
            invert3f(va);
          va+=3;
          
          /* go on to next vertex */
          
          v1+=3;
          vo+=3;
          ss++;
        }
        
        /* now iterate through pairs*/
        
        vo = pvo;
        va = pva;
        v  = nv; /* normals in direction of chain */
        
        for(a=1;a<nAt;a++) {
          
          v1 = va+6; /* orientation vectors for next CA */
          remove_component3f(vo  ,v,o0);
          normalize3f(o0);
          remove_component3f(v1  ,v,o1  ); 
          remove_component3f(v1+3,v,o1+3);
          normalize3f(o1);
          normalize3f(o1+3);
          max_dot = dot_product3f(o0,o1);
          v0 = v1;
          
          dp = dot_product3f(o0,o1+3);
          if(dp>max_dot) {
            v0 = v1+3;
            max_dot = dp;
          }
          
          copy3f(v0,vo+3); /* update with optimal orientation vector */
          
          vo+=3;
          va+=6; /* candidate orientation vectors */
          v+=3; /* normal */
        }


      }

      if(SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_flat_sheets)) {
        s = seg;
        v = pv;
        ss = sstype;
        vo = pvo;
        v0 = tv;
        last = 0;
        first = -1;
        end_flag=false;
        if(nAt>1) {
          for(a=0;a<nAt;a++) {

            if(a) {
              if(*s!=*(s-1)) { 
                end_flag=true;
              } else if(*ss!=2) {
                end_flag=true;
              }
              if(a==(nAt-1))
                end_flag=1;
            }
            if(end_flag) {
              
              for(f=1;f<2;f++) {
                for(c=0;c<3;c++) {
                  for(b=first+f;b<=last-f;b++) { /* iterative averaging */
                    zero3f(t0);
                    for(e=-f;e<=f;e++) {
                      add3f(pv+3*(b+e),t0,t0);
                    }
                    scale3f(t0,1.0/(f*2+1),tmp+b*3);
                  }
                  for(b=first+f;b<=last-f;b++) {
                    copy3f(tmp+b*3,pv+b*3);
                  }
                  for(b=first+f;b<=last-f;b++) { 
                    zero3f(t0);
                    for(e=-f;e<=f;e++) {
                      add3f(pvo+3*(b+e),t0,t0);
                    }
                    scale3f(t0,1.0/(f*2+1),tmp+b*3);
                  }
                  for(b=first+f;b<=last-f;b++) {
                    copy3f(tmp+b*3,pvo+b*3);
                    normalize3f(pvo+b*3);
                  }
                }

              }
              first = -1;
              last = -1;
              end_flag=false;
            }
            if(*ss==2) {
              if(first<0)
                first=a;
              last = a;
            } 
            v+=3;
            ss++;
            vo+=3;
            v0+=3;
            s++;
          }
        }

      }

      if(SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_smooth_loops)) {

        s = seg;
        v = pv;
        ss = sstype;
        vo = pvo;
        v0 = tv;
        last = 0;
        first = -1;
        end_flag=false;
        if(nAt>1) {
          for(a=0;a<nAt;a++) {

            if(a) {
              if(*s!=*(s-1)) { 
                end_flag=true;
              } else if(*ss!=0) {
                end_flag=true;
              }
              if(a==(nAt-1))
                end_flag=1;
            }
            if(end_flag) {
              if(a)
                if(first>0) /* 011130 WLD */
                  if(*(seg+first)==*(seg+first-1))
                    first--;
              if(last>0)
                if(*s==*(s-1))
                  if(last<(nAt-1)) 
                    last++;

              for(f=1;f<2;f++) {
                for(c=0;c<3;c++) {
                  for(b=first+f;b<=last-f;b++) { /* iterative averaging */
                    zero3f(t0);
                    for(e=-f;e<=f;e++) {
                      add3f(pv+3*(b+e),t0,t0);
                    }
                    scale3f(t0,1.0/(f*2+1),tmp+b*3);
                  }
                  for(b=first+f;b<=last-f;b++) {
                    copy3f(tmp+b*3,pv+b*3);
                  }
                  for(b=first+f;b<=last-f;b++) { 
                    zero3f(t0);
                    for(e=-f;e<=f;e++) {
                      add3f(pvo+3*(b+e),t0,t0);
                    }
                    scale3f(t0,1.0/(f*2+1),tmp+b*3);
                  }
                  for(b=first+f;b<=last-f;b++) {
                    copy3f(tmp+b*3,pvo+b*3);
                    normalize3f(pvo+b*3);
                  }
                }

              }
              first = -1;
              last = -1;
              end_flag=false;
            }
            if(*ss==0) {
              if(first<0)
                first=a;
              last = a;
            } 
            v+=3;
            ss++;
            vo+=3;
            v0+=3;
            s++;
          }
        }
      }
      
      if(SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_smooth_loops)||
         SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_flat_sheets)) {
        
        /* recompute differences and normals */
        
        s=seg;
        v=pv;
        v1=dv;
        v2=nv;
        d=dl;
        for(a=0;a<(nAt-1);a++)
          {
            if(*s==*(s+1))
              {
                subtract3f(v+3,v,v1);
                *d = length3f(v1);
                if(*d>R_SMALL4) {
                  scale3f(v1,1.0/(*d),v2);
                } else if(a)  {
                  copy3f(v2-3,v2); 
                } else {
                  zero3f(v2);
                }
              }
            else {
              zero3f(v2);	
            }
            d++;
            v+=3;
            v1+=3;
            v2+=3;
            s++;
          }
      
      
        /* recompute tangents */
        
        s=seg;
        v=nv;
        
        v1=tv;
        
        *(v1++)=*(v++); /* first segment */
        *(v1++)=*(v++);
        *(v1++)=*(v++);
        s++;
        
        for(a=1;a<(nAt-1);a++)
          {
            if((*s==*(s-1))&&(*s==*(s+1)))
              {
                add3f(v,(v-3),v1);
                normalize3f(v1);			 
              }
            else if(*s==*(s-1))
              {
                *(v1)=*(v-3);  /* end a segment */
                *(v1+1)=*(v-2); 
                *(v1+2)=*(v-1); 
              }
            else if(*s==*(s+1))
              {
                *(v1)=*(v);   /* new segment */
                *(v1+1)=*(v+1); 
                *(v1+2)=*(v+2); 
              }
            v+=3;
            v1+=3;
            s++;
          }
        
        *(v1++)=*(v-3); /* last segment */
        *(v1++)=*(v-2);
        *(v1++)=*(v-1);

        
        if(SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_flat_sheets)) {
          s=seg+1;
          ss = sstype+1;
          v2=tv+3; /* normal */
          for(a=1;a<(nAt-1);a++)
            {
              if((*ss==2)&&(*s==*(s+1))&&(*s==*(s-1))) { /* sheet in same segment */
                if((*ss==*(ss+1))&&(*ss!=*(ss-1))) { /* start, bias forwards */
                  scale3f(v2+3,refine_tips,t0);
                  add3f(t0,v2,v2);
                  normalize3f(v2);
                } else if((*ss!=*(ss+1))&&(*ss==*(ss-1)))  { /* end, bias backwards */
                  scale3f(v2-3,refine_tips,t0);
                  add3f(t0,v2,v2);
                  normalize3f(v2);
                }
              }
              
              v2+=3;
              s++;
              ss++;
            }
        }

      }
    }

  
  I->std = CGONew();

  /* debugging output */
  if(SettingGet_f(cs->Setting,obj->Obj.Setting,cSetting_cartoon_round_helices)) {
    if((cartoon_debug>0.5)&&(cartoon_debug<2.5)) {
      CGOColor(I->std,1.0,1.0,1.0);
      CGODisable(I->std,GL_LIGHTING);
      CGOBegin(I->std,GL_LINE_STRIP);
            
      v1 = NULL;
      v2 = NULL;
      v3 = NULL;
      v4 = NULL;
      v = pv;
      if(nAt>1) {
        CGOBegin(I->std,GL_LINE_STRIP);
        for(a=0;a<nAt;a++) {
          v4 = v3;
          v3 = v2;
          v2 = v1;
          v1 = v;
          if(v1&&v2&&v3&&v4) {
            add3f(v1,v4,t0);
            add3f(v2,v3,t1);
            scale3f(t0,0.2024,t0);
            scale3f(t1,0.2727,t1);
            add3f(t0,t1,t0);
            CGOVertexv(I->std,t0);
          }
          v+=3;
        }
        CGOEnd(I->std);
      }
    }
  }

PRINTFD(FB_RepCartoon)
" RepCartoon-Debug: creating 3D scaffold...\n"
ENDFD;
   
  /* okay, we now have enough info to generate smooth interpolations */


  if(nAt>1) {
    ex = ExtrudeNew();
    ExtrudeAllocPointsNormalsColors(ex,cs->NIndex*(3*sampling+3));
    n_p = 0;
    v = ex->p;
    vc = ex->c;
    vn = ex->n;
    
	 v1=pv; /* points */
	 v2=tv; /* tangents */
    vo=pvo;
    d = dl;
	 s=seg;
    cc=car;
	 atp=at;
    a=0;
    contFlag=true;
    cur_car = cCartoon_skip;
    extrudeFlag=false;
    contigFlag=false;

    while(contFlag) {


      if ((*cc)!=cur_car) { /* new cartoon type */
        if(n_p) { /* any cartoon points? */
          extrudeFlag=true;
        } else {
          cur_car = *(cc); /* no: go ahead and switch cartoons */
          ExtrudeTruncate(ex,0);
          n_p = 0;
          v = ex->p;
          vc = ex->c;
          vn = ex->n;
        }
      }
      if(a<nAt) {
        /* put a setting controlled conditional here.. */
        if (((*(cc+1))!=cur_car)&&(cur_car!=cCartoon_loop)) { /* end of segment */
          if(n_p) { /* any cartoon points? */
            extrudeFlag=true;
          } else {
            cur_car = cCartoon_loop; /* no: go ahead and switch cartoons */
            ExtrudeTruncate(ex,0);
            n_p = 0;
            v = ex->p;
            vc = ex->c;
            vn = ex->n;
          }
        }
      }
      if((a<(nAt-1))&&!extrudeFlag) {
        if((*s)!=*(s+1)) { /* new segment */
          contigFlag=false;
          if(n_p) { /* any cartoon points? */
            extrudeFlag=true;
          } else {
            ExtrudeTruncate(ex,0);
            n_p = 0;
            v = ex->p;
            vc = ex->c;
            vn = ex->n;
          }
        }
      }
      if(!extrudeFlag) {
		  if((a<(nAt-1))&&(*s==*(s+1))) /* working in the same segment... */
			 {
				c1=*(cs->Color+*atp);
				c2=*(cs->Color+*(atp+1));
            if(discrete_colors) {
              if(n_p==0) {
                if(contigFlag) {
                  if(cur_car!=cCartoon_loop)
                    c2=c1;
                  else {
                    if ((*cc+1)==cur_car)
                      c2=c1;
                    else
                      c1=c2;
                  }
                } else if((cur_car==cCartoon_loop)&&
                          (*(cc+1)!=cCartoon_loop)) {
                  c2=c1;
                }
              } else {
                if((cur_car==cCartoon_loop)&&
                   (*(cc+1)!=cCartoon_loop)) {
                  c2=c1;
                }
              }/* not contig */
              
            }
            dev = throw*(*d);
				for(b=0;b<sampling;b++) /* needs optimization */
				  {
                
                if(n_p==0) {
                  
                  /* provide starting point on first point in segment only... */

                  f0=((float)b)/sampling; /* fraction of completion */
                  f0=smooth(f0,power_a); /* bias sampling towards the center of the curve */
                  
                  if(f0<0.5) {
                    v0 = ColorGet(c1);
                  } else {
                    v0 = ColorGet(c2);
                  }

                  /* store colors */
                  
                  *(vc++)=*(v0++);
                  *(vc++)=*(v0++);
                  *(vc++)=*(v0++);

                  /* start of line/cylinder */
                  
                  f1=1.0-f0;
                  f2=smooth(f0,power_b);
                  f3=smooth(f1,power_b);
                  f4 = dev*f2*f3; /* displacement magnitude */

                  *(v++)=f1*v1[0]+f0*v1[3]+
                    f4*( f3*v2[0]-f2*v2[3] );

                  *(v++)=f1*v1[1]+f0*v1[4]+
                    f4*( f3*v2[1]-f2*v2[4] );

                  *(v++)=f1*v1[2]+f0*v1[5]+
                    f4*( f3*v2[2]-f2*v2[5] );

                  /* compute orientation vector at point, and store
                   in second position of axes */
                     
                  vn+=3;
                  *(vn++)=f1*(vo[0]*f2)+f0*(vo[3]*f3);
                  *(vn++)=f1*(vo[1]*f2)+f0*(vo[4]*f3);
                  *(vn++)=f1*(vo[2]*f2)+f0*(vo[5]*f3);     
                  vn+=3;
                  
                  copy3f(vo,vn-6); /* starter... */
                  n_p++;

                }

					 f0=((float)b+1)/sampling;
                f0=smooth(f0,power_a); /* bias sampling towards the center of the curve */                
                
                if(f0<0.5) {
                  v0 = ColorGet(c1);
                } else {
                  v0 = ColorGet(c2);
                }

                /* store colors */
                
                *(vc++)=*(v0++);
                *(vc++)=*(v0++);
                *(vc++)=*(v0++);
                
                /* end of line/cylinder */
                
                f1=1.0-f0;
                f2=smooth(f0,power_b);
                f3=smooth(f1,power_b);
                f4 = dev*f2*f3; /* displacement magnitude */
                
                *(v++)=f1*v1[0]+f0*v1[3]+
                  f4*( f3*v2[0]-f2*v2[3] );
                
                *(v++)=f1*v1[1]+f0*v1[4]+
                  f4*( f3*v2[1]-f2*v2[4] );
                
                *(v++)=f1*v1[2]+f0*v1[5]+
                  f4*( f3*v2[2]-f2*v2[5] );
                
                /*                remove_component3f(vo,v2,o0);
                                  remove_component3f(vo+3,v2,o0+3);*/
                
                vn+=3;
                *(vn++)=f1*(vo[0]*f2)+f0*(vo[3]*f3);
                *(vn++)=f1*(vo[1]*f2)+f0*(vo[4]*f3);
                *(vn++)=f1*(vo[2]*f2)+f0*(vo[5]*f3);                 
                vn+=3;
                
                if(b==sampling-1)
                  copy3f(vo+3,vn-6); /* starter... */                  
					 n_p++;
                
				  }

            /* now do a smoothing pass along orientation 
               vector to smooth helices, etc... */

            c = refine;
            cross_product3f(vn+3-(sampling*9),vn+3-9,t0);
            if((sampling>1)&&length3f(t0)>R_SMALL4) {

              normalize3f(t0);
              while(c--) {
                p0=v-(sampling*3)-3;
                p1=v-(sampling*3);
                p2=v-(sampling*3)+3;
                for(b=0;b<(sampling-1);b++) {
                  f0=dot_product3f(t0,p0);
                  f1=dot_product3f(t0,p1);
                  f2=dot_product3f(t0,p2);
                  
                  f3 = (f2+f0)/2.0;
                  scale3f(t0,f3-f1,t1);
                  add3f(t1,p1,p1);

                  f0=dot_product3f(t0,p0);
                  f1=dot_product3f(t0,p1);
                  f2=dot_product3f(t0,p2);
                  
                  p0=p1;
                  p1=p2;
                  p2+=3;
                }
              }
            }
			 }
		  v1+=3;
		  v2+=3;
		  v3+=3;
        vo+=3;
        d++;
		  atp+=1;
		  s++;
        cc++;

      }


      a++;
      if(a==nAt) {
        contFlag=false;
        if(n_p) 
          extrudeFlag=true;
      }
      if(extrudeFlag) {
        contigFlag=true;
        if((a<nAt)&&extrudeFlag) {
          if(*(s-1)!=*(s))
            contigFlag=false;
        }

        if(cur_car!=cCartoon_skip) {
          ExtrudeTruncate(ex,n_p);
          ExtrudeComputeTangents(ex);
          
          /* set up shape */
          switch(cur_car) {
          case cCartoon_tube:
            ExtrudeCircle(ex,tube_quality,tube_radius);
            ExtrudeBuildNormals1f(ex);
            ExtrudeCGOSurfaceTube(ex,I->std,1);
            break;
          case cCartoon_loop:
            ExtrudeCircle(ex,loop_quality,loop_radius);
            ExtrudeBuildNormals1f(ex);
            ExtrudeCGOSurfaceTube(ex,I->std,1);
            break;
          case cCartoon_rect:
            ExtrudeRectangle(ex,width,length);
            ExtrudeBuildNormals2f(ex);
            ExtrudeCGOSurfacePolygon(ex,I->std,1);
            break;
          case cCartoon_oval:
            ExtrudeOval(ex,oval_quality,oval_width,oval_length);
            ExtrudeBuildNormals2f(ex);
            ExtrudeCGOSurfaceTube(ex,I->std,1);
            break;
          case cCartoon_arrow:
            ExtrudeRectangle(ex,width,length);
            ExtrudeBuildNormals2f(ex);
            ExtrudeCGOSurfaceStrand(ex,I->std,sampling);
            break;
          case cCartoon_dumbbell:
            ExtrudeDumbbell1(ex,dumbbell_width,dumbbell_length);
            ExtrudeBuildNormals2f(ex);
            ExtrudeCGOSurfacePolygonTaper(ex,I->std,sampling);
            /*
                          ExtrudeCGOSurfacePolygonX(ex,I->std,1);*/

            ex1 = ExtrudeCopyPointsNormalsColors(ex);
            ExtrudeDumbbellEdge(ex1,sampling,-1,dumbbell_length);
            ExtrudeComputeTangents(ex1);
            ExtrudeCircle(ex1,loop_quality,dumbbell_radius);
            ExtrudeBuildNormals1f(ex1);
            ExtrudeCGOSurfaceTube(ex1,I->std,1);
            ExtrudeFree(ex1);

            ex1 = ExtrudeCopyPointsNormalsColors(ex);
            ExtrudeDumbbellEdge(ex1,sampling,1,dumbbell_length);
            ExtrudeComputeTangents(ex1);
            ExtrudeCircle(ex1,loop_quality,dumbbell_radius);
            ExtrudeBuildNormals1f(ex1);
            ExtrudeCGOSurfaceTube(ex1,I->std,1);
            ExtrudeFree(ex1);

            break;
          }
        }
        a--; /* undo above... */
        extrudeFlag=false;
        ExtrudeTruncate(ex,0);
        n_p = 0;
        v = ex->p;
        vc = ex->c;
        vn = ex->n;
      }
      }
    ExtrudeFree(ex); 
    if((cartoon_debug>0.5)&&(cartoon_debug<2.5)) {
      CGOColor(I->std,1.0,1.0,1.0);
      CGODisable(I->std,GL_LIGHTING);
      CGOBegin(I->std,GL_LINES);
      v1=pv;
      v2=pvo;
      v3=tv;
      for(a=0;a<nAt;a++) 
        {
          CGOVertexv(I->std,v1);
          add3f(v1,v2,t0);
          add3f(v2,t0,t0);
          CGOVertexv(I->std,t0);
          subtract3f(v1,v3,t0);
          CGOVertexv(I->std,t0);
          add3f(v1,v3,t0);
          CGOVertexv(I->std,t0);
          v1+=3;
          v2+=3;
          v3+=3;
        }
      CGOEnd(I->std);
      CGOEnable(I->std,GL_LIGHTING);
    }
  }
  

  CGOStop(I->std);
    
  FreeP(dv);
  FreeP(dl);
  FreeP(tv);
  FreeP(nv);
  FreeP(at);
  FreeP(seg);
  FreeP(pv);
  FreeP(pvo);
  FreeP(pva);
  FreeP(car);
  FreeP(tmp);
  FreeP(sstype);
  return((void*)(struct Rep*)I);
}


  
