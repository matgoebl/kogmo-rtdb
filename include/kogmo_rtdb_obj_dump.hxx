//TODO!
#ifdef _NEED_NEW_ANYDUMP
// perl -ne '/^class *([^\s]+)/ && print "  DUMPTRY($1);\n"' kogmo_objects/*.hxx
namespace KogniMobil {
 inline std::string anydump(RTDBObj &dumpobj)
 {
   RTDBConn DBC(dumpobj.getDBC());
   #define DUMPTRY(OBJ) { OBJ Obj(DBC); if ( Obj.Copy(dumpobj,1) ) return Obj.dump(); }
   // Der Trick: Obj.Copy() liefert nur true, wenn die TYPE-ID matcht
   DUMPTRY(C3_RTDB);
   DUMPTRY(C3_Process);
   DUMPTRY(C3_Text);
   DUMPTRY(C3_PlayerCtrl);
   DUMPTRY(C3_PlayerStat);
   DUMPTRY(C3_RecorderStat);
   //DUMPTRY(YOUR_OBJECT_WITH_A_DUMP_METHOD);
   DUMPTRY(A2_AnyImage); // das steht am Ende, da es "testweise" recht viel Speicher alloziert
   // nach Ergaenzungen muss man qrtdbctl neu compilieren
   return dumpobj.dump();
 }
}
#endif
