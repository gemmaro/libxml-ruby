#include "ruby_libxml.h"
#include "ruby_xml_dtd.h"

/*
 * Document-class: LibXML::XML::Dtd
 *
 * The XML::Dtd class is used to prepare DTD's for validation of xml
 * documents.
 *
 * DTDs can be created from a string or a pair of public and system identifiers.
 * Once a Dtd object is instantiated, an XML document can be validated by the
 * XML::Document#validate method providing the XML::Dtd object as parameeter.
 * The method will raise an exception if the document is
 * not valid.
 *
 * Basic usage:
 *
 *  # parse DTD
 *  dtd = XML::Dtd.new(<<EOF)
 *  <!ELEMENT root (item*) >
 *  <!ELEMENT item (#PCDATA) >
 *  EOF
 *
 *  # parse xml document to be validated
 *  instance = XML::Document.file('instance.xml')
 *
 *  # validate
 *  instance.validate(dtd)
 */

VALUE cXMLDtd;

void rxml_dtd_free(xmlDtdPtr xdtd)
{
  if (xdtd->doc == NULL && xdtd->parent == NULL)
    xmlFreeDtd(xdtd);
}

void rxml_dtd_mark(xmlDtdPtr xdtd)
{
  if (xdtd && xdtd->doc)
  {
      VALUE doc = (VALUE)xdtd->doc->_private;
      rb_gc_mark(doc);
  }
}

static VALUE rxml_dtd_alloc(VALUE klass)
{
  return Data_Wrap_Struct(klass, rxml_dtd_mark, rxml_dtd_free, NULL);
}

VALUE rxml_dtd_wrap(xmlDtdPtr xdtd)
{
  return  Data_Wrap_Struct(cXMLDtd, NULL, NULL, xdtd);
}

/*
 * call-seq:
 *    dtd.external_id -> "string"
 *
 * Obtain this dtd's external identifer (for a PUBLIC DTD).
 */
static VALUE rxml_dtd_external_id_get(VALUE self)
{
  xmlDtdPtr xdtd;
  Data_Get_Struct(self, xmlDtd, xdtd);


  if (xdtd->ExternalID == NULL)
    return (Qnil);
  else
    return (rxml_new_cstr( xdtd->ExternalID, NULL));
}

/*
 * call-seq:
 *    dtd.name -> "string"
 *
 * Obtain this dtd's name.
 */
static VALUE rxml_dtd_name_get(VALUE self)
{
  xmlDtdPtr xdtd;
  Data_Get_Struct(self, xmlDtd, xdtd);


  if (xdtd->name == NULL)
    return (Qnil);
  else
    return (rxml_new_cstr( xdtd->name, NULL));
}


/*
 * call-seq:
 *    dtd.uri -> "string"
 *
 * Obtain this dtd's URI (for a SYSTEM or PUBLIC DTD).
 */
static VALUE rxml_dtd_uri_get(VALUE self)
{
  xmlDtdPtr xdtd;
  Data_Get_Struct(self, xmlDtd, xdtd);

  if (xdtd->SystemID == NULL)
    return (Qnil);
  else
    return (rxml_new_cstr( xdtd->SystemID, NULL));
}

/*
 * call-seq:
 *    node.type -> num
 *
 * Obtain this node's type identifier.
 */
static VALUE rxml_dtd_type(VALUE self)
{
  xmlDtdPtr xdtd;
  Data_Get_Struct(self, xmlDtd, xdtd);
  return (INT2NUM(xdtd->type));
}

/*
 * call-seq:
 *    XML::Dtd.new(dtd_string) -> dtd
 *    XML::Dtd.new(external_id, system_id) -> dtd
 *    XML::Dtd.new(external_id, system_id, name, document, internal) -> dtd
 *
 * Create a new Dtd from the specified public and system identifiers:
 *   
 *   * The first usage creates a DTD from a string and requires 1 parameter.
 *   * The second usage loads and parses an external DTD and requires 2 parameters.
 *   * The third usage creates a new internal or external DTD and requires 2 parameters and 3 optional parameters.
 *     The DTD is then attached to the specified document if it is not nil.
 * 
 * Parameters:
 * 
 *   dtd_string - A string that contains a complete DTD
 *   external_id - A string that specifies the DTD's external name. For example, "-//W3C//DTD XHTML 1.0 Transitional//EN"
 *   system_id - A string that specififies the DTD's system name. For example, "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
 *   name - A string that specifies the DTD's name. For example "xhtml1".
 *   document - A xml document.
 *   internal - Boolean value indicating whether this is an internal or external DTD. Optional. If not specified
 *              then external is assumed.
 */
static VALUE rxml_dtd_initialize(int argc, VALUE *argv, VALUE self)
{
  xmlDtdPtr xdtd;
  VALUE external, system;

  switch (argc)
  {
      case 3:
      case 4:
      case 5:
      {
          const xmlChar *xname = NULL, *xpublic = NULL, *xsystem = NULL;
          xmlDocPtr xdoc = NULL;

          VALUE name, doc, internal;
          rb_scan_args(argc, argv, "23", &external, &system, &name, &doc, &internal);

          Check_Type(external, T_STRING);
          xpublic = (const xmlChar*) StringValuePtr(external);

          Check_Type(system, T_STRING);
          xsystem = (const xmlChar*) StringValuePtr(system);

          if (name != Qnil)
          {
            Check_Type(name, T_STRING);
            xname = (const xmlChar*)StringValuePtr(name);
          }

          if (doc != Qnil)
          {
            if (rb_obj_is_kind_of(doc, cXMLDocument) == Qfalse)
              rb_raise(rb_eTypeError, "Must pass an LibXML::XML::Document object");
            Data_Get_Struct(doc, xmlDoc, xdoc);
          }

          if (internal == Qnil || internal == Qfalse)
            xdtd = xmlNewDtd(xdoc, xname, xpublic, xsystem);
          else
            xdtd = xmlCreateIntSubset(xdoc, xname, xpublic, xsystem);

          if (xdtd == NULL)
            rxml_raise(xmlGetLastError());

          /* The document will free the dtd so Ruby should not */
          RDATA(self)->dfree = NULL;
          DATA_PTR(self) = xdtd;

          xmlSetTreeDoc((xmlNodePtr) xdtd, xdoc);
        }
        break;

      case 2:
      {
        rb_scan_args(argc, argv, "20", &external, &system);

        Check_Type(external, T_STRING);
        Check_Type(system, T_STRING);

        xdtd = xmlParseDTD((xmlChar*) StringValuePtr(external), (xmlChar*) StringValuePtr(system));

        if (xdtd == NULL)
          rxml_raise(xmlGetLastError());

        DATA_PTR(self) = xdtd;

        xmlSetTreeDoc((xmlNodePtr) xdtd, NULL);
        break;
      }
      case 1:
      {
        VALUE dtd_string;
        rb_scan_args(argc, argv, "10", &dtd_string);
        Check_Type(dtd_string, T_STRING);

        /* Note that buffer is freed by xmlParserInputBufferPush*/
        xmlCharEncoding enc = XML_CHAR_ENCODING_NONE;
        xmlParserInputBufferPtr buffer = xmlAllocParserInputBuffer(enc);
        xmlChar *new_string = xmlStrdup((xmlChar*) StringValuePtr(dtd_string));
        xmlParserInputBufferPush(buffer, xmlStrlen(new_string),
            (const char*) new_string);

        xdtd = xmlIOParseDTD(NULL, buffer, enc);

        if (xdtd == NULL)
          rxml_raise(xmlGetLastError());

        xmlFree(new_string);

        DATA_PTR(self) = xdtd;
        break;
      }
      default:
        rb_raise(rb_eArgError, "wrong number of arguments");
  }

  return self;
}

void rxml_init_dtd(void)
{
  cXMLDtd = rb_define_class_under(mXML, "Dtd", rb_cObject);
  rb_define_alloc_func(cXMLDtd, rxml_dtd_alloc);
  rb_define_method(cXMLDtd, "initialize", rxml_dtd_initialize, -1);
  rb_define_method(cXMLDtd, "external_id", rxml_dtd_external_id_get, 0);
  rb_define_method(cXMLDtd, "name", rxml_dtd_name_get, 0);
  rb_define_method(cXMLDtd, "uri", rxml_dtd_uri_get, 0);
  rb_define_method(cXMLDtd, "node_type", rxml_dtd_type, 0);
  rb_define_alias(cXMLDtd, "system_id", "uri");
}
