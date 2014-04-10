%module nametag_java

%include "../common/nametag.i"

%pragma(java) jniclasscode=%{
  static {
    java.io.File localNametag = new java.io.File(System.mapLibraryName("nametag_java"));

    if (localNametag.exists())
      System.load(localNametag.getAbsolutePath());
    else
      System.loadLibrary("nametag_java");
  }
%}
