%include "../nametag.i"

%pragma(java) jniclasscode=%{
  static {
    System.loadLibrary("nametag_java");
  }
%}
