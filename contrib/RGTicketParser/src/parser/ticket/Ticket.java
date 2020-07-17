package parser.ticket;

/**
 * Entity class representing base values of a redmine ticket
 * @author Pointerx
 */
public class Ticket {
    private int Id;
    private String priority;
    private String subject;
    private String description;
    private String status;
    private int questId;

    public Ticket() {}
    
    public Ticket (int id, String prio, String subj, String desc, String status)
    {
        this.Id = id;
        this.priority = prio;
        this.subject = subj;
        this.description = desc;
        this.status = status;
    }
    
    /**
     * @return the Id
     */
    public int getId() {
        return Id;
    }

    /**
     * @param Id the Id to set
     */
    public void setId(int Id) {
        this.Id = Id;
    }

    /**
     * @return the priority
     */
    public String getPriority() {
        return priority;
    }

    /**
     * @param priority the priority to set
     */
    public void setPriority(String priority) {
        this.priority = priority;
    }

    /**
     * @return the subject
     */
    public String getSubject() {
        return subject;
    }

    /**
     * @param subject the subject to set
     */
    public void setSubject(String subject) {
        this.subject = subject;
    }

    /**
     * @return the description
     */
    public String getDescription() {
        return description;
    }

    /**
     * @param description the description to set
     */
    public void setDescription(String description) {
        this.description = description;
    }

    /**
     * @return the status
     */
    public String getStatus() {
        return status;
    }

    /**
     * @param status the status to set
     */
    public void setStatus(String status) {
        this.status = status;
    }

    /**
     * @return the questId
     */
    public int getQuestId() {
        return questId;
    }

    /**
     * @param questId the questId to set
     */
    public void setQuestId(int questId) {
        this.questId = questId;
    }
    
}
