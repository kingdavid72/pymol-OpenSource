
from pymol.wizard import Wizard
from pymol import cmd
import pymol
import copy

default_map = [ '', '', '']
default_level = [ 1.0, 3.0, -3.0]
default_radius = 8.0
default_track = 0

class Density(Wizard):

    def __init__(self):

        cmd.unpick()
        
        Wizard.__init__(self)
        
        # mode selection subsystem
        
        self.radius = default_radius
        self.map = copy.deepcopy(default_map)
        self.level = copy.deepcopy(default_level)
        self.track = copy.deepcopy(default_track)
        self.avail_maps = []
        
        self.menu['radius'] = [
                                      [1, '4.0 A Radius','cmd.get_wizard().set_radius(4)'],
                                      [1, '5.0 A Radius','cmd.get_wizard().set_radius(5)'],
                                      [1, '6.0 A Radius','cmd.get_wizard().set_radius(6)'],
                                      [1, '8.0 A Radius','cmd.get_wizard().set_radius(8)'],
                                      [1, '10.0 A Radius','cmd.get_wizard().set_radius(10)'],
                                      [1, '15.0 A Radius','cmd.get_wizard().set_radius(15)'],
                                      [1, '20.0 A Radius','cmd.get_wizard().set_radius(20)'],
                                      [1, '50.0 A Radius','cmd.get_wizard().set_radius(50)'],
                                      ]
                                        
        self.menu['map0'] = []
        self.menu['map1'] = []
        self.menu['map2'] = []

        level_menu = lambda x:[ [1, '1.0 sigma','cmd.get_wizard().set_level(%d,1.0)'%x],
                                        [1, '1.5 sigma','cmd.get_wizard().set_level(%d,1.5)'%x],
                                        [1, '2.0 sigma','cmd.get_wizard().set_level(%d,2.0)'%x],
                                        [1, '3.0 sigma','cmd.get_wizard().set_level(%d,3.0)'%x],
                                        [1, '5.0 sigma','cmd.get_wizard().set_level(%d,5.0)'%x],
                                        [1, '-3.0 sigma','cmd.get_wizard().set_level(%d,-3.0)'%x]]
        
        self.menu['level0'] = level_menu(0)
        self.menu['level1'] = level_menu(1)
        self.menu['level2'] = level_menu(2)
        
        self.menu['track'] = [
        [ 1, "Track & Zoom", 'cmd.get_wizard().set_track(0)'],
        [ 1, "Track & Center", 'cmd.get_wizard().set_track(1)'],
        [ 1, "Track & Set Origin", 'cmd.get_wizard().set_track(2)'],
        [ 1, "Track Off", 'cmd.get_wizard().set_track(3)'],
        ]

        if (self.map[0] == '') and (self.map[1] == '') and (self.map[2]==''):
            for a in cmd.get_names(): # automatically load first map we find
                if cmd.get_type(a)=='object:map':
                    self.map[0]=a
                    break
        self.update_map_menus()

        cmd.set_key('pgup',lambda c=cmd:c.get_wizard().next_res(d=-1))
        cmd.set_key('pgdn',lambda c=cmd:c.get_wizard().next_res())      
        
    def update_map_menus(self):

        self.avail_maps = []
        
        for a in cmd.get_names('objects'):
            if cmd.get_type(a)=='object:map':
                self.avail_maps.append(a)

        c = 0
        for a in self.map:
            map_kee = 'map'+str(c)
            level_kee = 'level'+str(c)
            self.menu[map_kee] = [[2,'Select Map','']]
            for a in self.avail_maps:
                self.menu[map_kee].append([ 1,a,'cmd.get_wizard().set_map(%d,"%s")'%(c,a) ])
            self.menu[map_kee].append([1,'(none)','cmd.get_wizard().set_map(%d,"")'%c])
            c = c + 1

    def set_track(self,track):
        self.track = track
        cmd.refresh_wizard()
        
    def set_level(self,map,level):
        self.level[map] = level
        self.update_maps()
        cmd.refresh_wizard()

    def set_map(self,map,map_name):
        self.map[map] = map_name
        cmd.refresh_wizard()

    def set_radius(self,radius):
        self.radius = radius
        self.update_maps()
        cmd.refresh_wizard()

    def update_maps(self):
        if '_dw' in cmd.get_names('selections'):
            save = cmd.get_setting_text('auto_zoom')
            save = cmd.set('auto_zoom',0,quiet=1)                     
            c = 0
            for a in self.map:
                oname = 'w'+str(c+1)+'_'+a
                if oname not in cmd.get_names():
                    color = 1
                else:
                    color = 0
                if len(a) and (a in cmd.get_names('objects')):
                    if cmd.get_type(a)=='object:map':
                        cmd.isomesh(oname,a,self.level[c],
                                        "(_dw)",self.radius,state=1)
                    if color:
                        if c == 0:
                            cmd.color('blue',oname)
                        elif c == 1:
                            cmd.color('white',oname)
                        else:
                            cmd.color('magenta',oname)
                c = c + 1
            save = cmd.set('auto_zoom',save,quiet=1)            
            if self.track==0:
                cmd.zoom("(_dw)",self.radius)
            elif self.track==1:
                cmd.center("(_dw)")
            elif self.track==2:
                cmd.origin("(_dw)")
        cmd.refresh_wizard()      
# generic set routines

    def zoom(self):
        if '_dw' in cmd.get_names('selections'):
            cmd.zoom("(_dw)",self.radius)
            cmd.clip("slab",self.radius-1)
        else:
            c = 0
            for a in self.map:
                oname = 'w'+str(c+1)+'_'+a
                if len(a):
                    if a in cmd.get_names('objects'):
                        cmd.zoom(oname)
                        cmd.clip("slab",self.radius-1)
                c = c + 1
                
    def get_panel(self):
        self.update_map_menus()
        return [
            [ 1, 'Density Map Wizard',''],
            [ 2, 'Update Maps' , 'cmd.get_wizard().update_maps()'],
            [ 2, 'Zoom' , 'cmd.get_wizard().zoom()'],         
            [ 2, 'Next Res. (PgDown)' , 'cmd.get_wizard().next_res()'],
            [ 2, 'Previous Res. (PgUp)' , 'cmd.get_wizard().next_res(d=-1)'],
            [ 3, "Radius: %3.1f A"%self.radius,'radius'],
            [ 3, "Map 1: "+self.map[0],'map0'],
            [ 3, "       @ "+str(self.level[0])+" sigma",'level0'],
            [ 3, "Map 2: "+self.map[1],'map1'],
            [ 3, "       @ "+str(self.level[1])+" sigma",'level1'],
            [ 3, "Map 3: "+self.map[2],'map2'],
            [ 3, "       @ "+str(self.level[2])+" sigma",'level2'],
            [ 3, self.menu['track'][self.track][1], 'track' ],
            [ 2, 'Done','cmd.set_wizard()'],
            ]

    def cleanup(self):
        global default_radius,default_map,default_level
        default_radius = self.radius
        default_map = self.map
        default_level = self.level
        default_track = self.track
        self.clear()
        cmd.set_key('pgup',None)
        cmd.set_key('pgdn',None)
        
    def clear(self):
        pass

    def do_pick(self,bondFlag):
        global dist_count
        if not bondFlag:
            if self.track!=2:
                cmd.select("_dw","pk1",quiet=1)
                self.update_maps()
                cmd.unpick()
                
    def next_res(self, d=1):
        # Donated by Tom Lee
        if not cmd.count_atoms('?_dw'):
            if cmd.count_atoms("?pk1"):
                cmd.select("_dw","pk1")
        if not ('_dw' in cmd.get_names('selections')):
            print " Density-Wizard: Please pick an atom first."
        else:
            obj = cmd.index('_dw')[0][0]
            a0 = cmd.get_model('_dw').atom[0]
            cmd.select("_res0", "byres (_dw)")
            res0 = cmd.get_model("_res0")
            atn = a0.name
            for a in res0.atom:
                if (a.name == 'CA'):
                    atn = 'CA'
                    break
                elif (a.name == 'C1*'):
                    atn = 'C1*'
                    break
                elif (a.name == 'C1\''):
                    atn = 'C1\''
                    break
            n = cmd.select('_dw2', ''+obj+'/'+a0.segi+'/'+a0.chain+'/'+str(a0.resi_number+d)+'/'+atn)
            if (n == 0):  # deal with gaps in sequence:
                cmd.select('_chain', ''+obj+'/'+a0.segi+'/'+a0.chain+'//'+atn)
                chain = cmd.get_model('_chain')
                resids = []
                for a in chain.atom:
                    resids.append(a.resi)
                    if (a.resi_number == a0.resi_number):
                        i = len(resids) - 1
                next_i = i + d
                if ((next_i < 0) or (next_i >= len(resids))):
                    print "Current residue is the end of a chain."
                else:
                    n = cmd.select('_dw2', ''+obj+'/'+a0.segi+'/'+a0.chain+'/'+resids[next_i]+'/'+atn)
            if (n > 0):
                cmd.hide("labels", "?_dw") 
                cmd.select('dw_resi', 'byres _dw2')
                cmd.disable('dw_resi')
                cmd.label('(_dw2)', '"  %s %s/%s/" % (resn,chain,resi)')
                cmd.select('_dw','_dw2')
                cmd.delete('_dw2')
                self.update_maps()
