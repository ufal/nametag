%module nametag_java

%include "../common/nametag.i"

%pragma(java) jniclasscode=%{
  static {
    java.io.File libraryFile = new java.io.File(nametag_java.libraryPath);

    if (libraryFile.exists())
      System.load(libraryFile.getAbsolutePath());
    else
      System.loadLibrary("nametag_java");
  }
%}

%pragma(java) modulecode=%{
  static String libraryPath;

  static {
    libraryPath = System.mapLibraryName("nametag_java");
  }

  public static void setLibraryPath(String path) {
    libraryPath = path;
  }
%}
