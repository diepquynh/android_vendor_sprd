/**
 * Add for oma download
 *@{
 */
package com.android.browser;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import java.io.StringReader;
import android.util.Log;
import java.io.*;

/**
 * This {@link file} handles to parser the XML document
 */
public class BrowserXmlParser {
    Element root;

    public BrowserXmlParser(String fileName) {
        try {
            BufferedReader reader = new BufferedReader(new FileReader(fileName));

            String s, xml = new String();
            /* read this xml file */
            //fix bug 175468 on 20130607 start
            try {
                while ((s = reader.readLine()) != null)
                    xml += s + "\n";
            } catch (IOException e) {
                // TODO: handle exception
                Log.w("BrowserXmlParser", "BufferedReader readline() cause the IOException");
            }finally{
                reader.close();
            }
            //fix bug 175468 on 20130607 end

            DocumentBuilderFactory xml_doc_factory = null;
            DocumentBuilder xml_dBuilder = null;
            Document xml_doc = null;

            /*
             * now parse this XML file, and read the node info which we
             * need,such as object url
             */
            xml_doc_factory = DocumentBuilderFactory.newInstance();
            xml_dBuilder = xml_doc_factory.newDocumentBuilder();
            InputSource source = new InputSource(new StringReader(xml.replace(
                    '&', '*')));
            xml_doc = xml_dBuilder.parse(source);
            root = xml_doc.getDocumentElement();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public String getValue(String key) {
        if (root == null)
            return null;
        NodeList xml_node = root.getElementsByTagName(key);
        if (xml_node == null)
            return null;
        // fix NullPointException 2011-05-24 yangwu
        String values = null;
        try {
            values = xml_node.item(0).getFirstChild().getNodeValue()
                    .replace('*', '&');
        } catch (Exception e) {
            Log.w("BrowserXmlParser", "failed to get value of " + key);
            e.printStackTrace();
            return values;
        }
        return values;
    }

    public String getValueSecond(String key) {
        if (root == null)
            return null;
        NodeList xml_node = root.getElementsByTagName(key);
        if (xml_node == null)
            return null;
        // fix NullPointException 2011-05-24 yangwu
        String values = null;
        try {
            values = xml_node.item(1).getFirstChild().getNodeValue()
                    .replace('*', '&');
        } catch (Exception e) {
            Log.w("BrowserXmlParser", "failed to get value of " + key);
            e.printStackTrace();
            return null;
        }
        return values;
    }
}
