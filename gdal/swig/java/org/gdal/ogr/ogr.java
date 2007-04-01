/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.28
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.gdal.ogr;

import org.gdal.osr.SpatialReference;

public class ogr implements ogrConstants {
  public static Geometry CreateGeometryFromWkb(byte[] nLen, SpatialReference reference) {
    long cPtr = ogrJNI.CreateGeometryFromWkb(nLen, SpatialReference.getCPtr(reference));
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public static Geometry CreateGeometryFromWkt(String val, SpatialReference reference) {
    long cPtr = ogrJNI.CreateGeometryFromWkt(val, SpatialReference.getCPtr(reference));
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public static Geometry CreateGeometryFromGML(String input_string) {
    long cPtr = ogrJNI.CreateGeometryFromGML(input_string);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public static int GetDriverCount() {
    return ogrJNI.GetDriverCount();
  }

  public static int GetOpenDSCount() {
    return ogrJNI.GetOpenDSCount();
  }

  public static int SetGenerate_DB2_V72_BYTE_ORDER(int bGenerate_DB2_V72_BYTE_ORDER) {
    return ogrJNI.SetGenerate_DB2_V72_BYTE_ORDER(bGenerate_DB2_V72_BYTE_ORDER);
  }

  public static void RegisterAll() {
    ogrJNI.RegisterAll();
  }

  public static DataSource GetOpenDS(int ds_number) {
    long cPtr = ogrJNI.GetOpenDS(ds_number);
    return (cPtr == 0) ? null : new DataSource(cPtr, false);
  }

  public static DataSource Open(String filename, int update) {
    long cPtr = ogrJNI.Open(filename, update);
    return (cPtr == 0) ? null : new DataSource(cPtr, true);
  }

  public static DataSource OpenShared(String filename, int update) {
    long cPtr = ogrJNI.OpenShared(filename, update);
    return (cPtr == 0) ? null : new DataSource(cPtr, true);
  }

  public static Driver GetDriverByName(String name) {
    long cPtr = ogrJNI.GetDriverByName(name);
    return (cPtr == 0) ? null : new Driver(cPtr, false);
  }

  public static Driver GetDriver(int driver_number) {
    long cPtr = ogrJNI.GetDriver(driver_number);
    return (cPtr == 0) ? null : new Driver(cPtr, false);
  }

  public static int sum(int a, int b) {
    return ogrJNI.sum(a, b);
  }

}
