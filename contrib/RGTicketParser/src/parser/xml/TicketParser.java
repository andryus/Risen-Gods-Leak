package parser.xml;

import java.util.ArrayList;
import java.util.List;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.DocumentBuilder;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.w3c.dom.Node;
import org.w3c.dom.Element;
import parser.ticket.Ticket;

/**
 * Converts XML into valid Ticket classes using DOM Reader and JAX-RS
 * @author Pointerx
 */
public class TicketParser {
    
    public static int getCount()
    {
        try
        {
            DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
            DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
            Document doc = dBuilder.parse("http://redmine.rising-gods.de/issues.xml?project_id=1&limit=1&offset=0&category_id=12");


            return Integer.parseInt(doc.getDocumentElement().getAttribute("total_count"));
        }
        catch (Exception e)
        {
            System.out.println(e);
        }
        
        return 0;
    }

    public static List<Ticket> getTickets() {
        List<Ticket> tickets = new ArrayList<Ticket>();
        int max_count = TicketParser.getCount();
        for (int i = 0; i <= max_count; i+=100)
        {
            try
            {
                DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
                DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
                Document doc = dBuilder.parse("http://redmine.rising-gods.de/issues.xml?project_id=1&limit=100&offset="+i+"&category_id=12");

                NodeList nList = doc.getElementsByTagName("issue");
                for (int j = 0; j < nList.getLength(); j++)
                {
                    Element node = (Element)nList.item(j);

                    if (node.getNodeType() == Node.ELEMENT_NODE)
                    {
                        Ticket ticket = new Ticket();
                        ticket.setId(Integer.parseInt(node.getElementsByTagName("id").item(0).getTextContent()));
                        ticket.setDescription(node.getElementsByTagName("description").item(0).getTextContent());
                        ticket.setSubject(node.getElementsByTagName("subject").item(0).getTextContent());
                        ticket.setStatus(((Element)node.getElementsByTagName("status").item(0)).getAttribute("name"));
                        ticket.setPriority(((Element)node.getElementsByTagName("priority").item(0)).getAttribute("name"));
                        tickets.add(ticket);
                    }

                }
            }
            catch(Exception e)
            {
                System.out.println(e);
            }
        }
        return tickets;
    }
    
    
    
}
